#include "Common.hlsli"

Texture2D texture[50] : register(t0, space0);
TextureCube skyboxTexture[10] : register(t50, space0);

static const uint textureSizeOffset = 50;
static const float3 Fdielectric = 0.04; // 비금속(Dielectric) 재질의 F0

#define NEAR_PLANE 0.1
// #define LIGHT_WORLD_RADIUS 0.001
#define LIGHT_FRUSTUM_WIDTH 0.34641 // <- 계산해서 찾은 값

float3 SchlickFresnel(float3 F0, float NdotH)
{
    return F0 + (1.0 - F0) * pow(2.0, (-5.55473 * NdotH - 6.98316) * NdotH);
}

float3 GetNormal(PSInput input)
{
    float3 normalWorld = input.normalWorld;
    
    if (useNormalMap && normalIndex != 0) // NormalWorld를 교체
    {
        float3 normal = texture[normalIndex].SampleLevel(linearWrapSampler, input.texcoord, meshLodBias).rgb;
        normal = 2.0 * normal - 1.0; // 범위 조절 [-1.0, 1.0]

        // OpenGL 용 노멀맵일 경우에는 y 방향을 뒤집어줍니다.
        normal.y = invertNormalMapY ? -normal.y : normal.y;
        
        float3 N = normalWorld;
        float3 T = normalize(input.tangentWorld - dot(input.tangentWorld, N) * N);
        float3 B = cross(N, T);
        
        // matrix는 float4x4, 여기서는 벡터 변환용이라서 3x3 사용
        float3x3 TBN = float3x3(T, B, N);
        normalWorld = normalize(mul(normal, TBN));
    }
    
    return normalWorld;
}

float3 DiffuseIBL(float3 albedo, float3 normalWorld, float3 pixelToEye,
                  float metallic)
{
    float3 F0 = lerp(Fdielectric, albedo, metallic);
    float3 F = SchlickFresnel(F0, max(0.0, dot(normalWorld, pixelToEye)));
    float3 kd = lerp(1.0 - F, 0.0, metallic);
    
    // 앞에서 사용했던 방법과 동일
    float3 irradiance = skyboxTexture[cubemapDiffuseIndex - textureSizeOffset].SampleLevel(linearWrapSampler, normalWorld, 0.0).rgb;
    
    return kd * albedo * irradiance;
}

float3 SpecularIBL(float3 albedo, float3 normalWorld, float3 pixelToEye,
                   float metallic, float roughness)
{
    float2 specularBRDF = texture[brdfIndex].SampleLevel(linearClampSampler, float2(dot(normalWorld, pixelToEye), 1.0 - roughness), 0.0).rg;
    float3 specularIrradiance = skyboxTexture[cubemapSpecularIndex - textureSizeOffset].
        SampleLevel(linearWrapSampler, reflect(-pixelToEye, normalWorld), 2 + roughness * 5.0f).rgb;
    
    // 앞에서 사용했던 방법과 동일
    const float3 Fdielectric = 0.04; // 비금속(Dielectric) 재질의 F0
    float3 F0 = lerp(Fdielectric, albedo, metallic);

    return (F0 * specularBRDF.x + specularBRDF.y) * specularIrradiance;
}

float3 AmbientLightingByIBL(float3 albedo, float3 normalW, float3 pixelToEye, float ao,
                            float metallic, float roughness)
{
    float3 diffuseIBL = DiffuseIBL(albedo, normalW, pixelToEye, metallic);
    float3 specularIBL = SpecularIBL(albedo, normalW, pixelToEye, metallic, roughness);
    
    return (diffuseIBL + specularIBL) * ao;
}

// GGX/Towbridge-Reitz normal distribution function.
// Uses Disney's reparametrization of alpha = roughness^2.
float NdfGGX(float NdotH, float roughness)
{
    float alpha = roughness * roughness;
    float alphaSq = alpha * alpha;
    float denum = (NdotH * NdotH) * (alphaSq - 1.0) + 1.0;
    
    return alphaSq / (3.141592 * denum * denum);

}

float SchlickG1(float NdotV, float k)
{
    return NdotV / (NdotV * (1.0 - k) + k);

}

float SchlickGGX(float NdotI, float NdotO, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8;
    
    return SchlickG1(NdotI, k) * SchlickG1(NdotO, k);

}

float PCF_Filter(float2 uv, float zReceiverNdc, float filterRadiusUV, Texture2D shadowMap)
{
    float sum = 0.0f;
    for (int i = 0; i < 64; ++i)
    {
        float2 offset = diskSamples64[i] * filterRadiusUV;
        sum += shadowMap.SampleCmpLevelZero(
            shadowCompareSampler, uv + offset, zReceiverNdc);
    }
    return sum / 64;
}

// NdcDepthToViewDepth
float N2V(float ndcDepth, matrix invProj)
{
    float4 pointView = mul(float4(0, 0, ndcDepth, 1), invProj);
    return pointView.z / pointView.w;
}

void FindBlocker(out float avgBlockerDepthView, out float numBlockers, float2 uv,
                 float zReceiverView, Texture2D shadowMap, matrix invProj, float lightRadiusWorld)
{
    float lightRadiusUV = lightRadiusWorld / LIGHT_FRUSTUM_WIDTH;
    
    float searchRadius = lightRadiusUV * (zReceiverView - NEAR_PLANE) / zReceiverView;

    float blockerSum = 0;
    numBlockers = 0;
    for (int i = 0; i < 64; ++i)
    {
        float shadowMapDepth =
            shadowMap.SampleLevel(shadowPointSampler, float2(uv + diskSamples64[i] * searchRadius), 0).r;

        shadowMapDepth = N2V(shadowMapDepth, invProj);
        
        if (shadowMapDepth < zReceiverView)
        {
            blockerSum += shadowMapDepth;
            numBlockers++;
        }
    }
    avgBlockerDepthView = blockerSum / numBlockers;
}

float PCSS(float2 uv, float zReceiverNdc, Texture2D shadowMap, matrix invProj, float lightRadiusWorld)
{
    float lightRadiusUV = lightRadiusWorld / LIGHT_FRUSTUM_WIDTH;
    
    float zReceiverView = N2V(zReceiverNdc, invProj);
    
    // STEP 1: blocker search
    float avgBlockerDepthView = 0;
    float numBlockers = 0;

    FindBlocker(avgBlockerDepthView, numBlockers, uv, zReceiverView, shadowMap, invProj, lightRadiusWorld);

    if (numBlockers < 1)
    {
        // There are no occluders so early out(this saves filtering)
        return 1.0f;
    }
    else
    {
        // STEP 2: penumbra size
        float penumbraRatio = (zReceiverView - avgBlockerDepthView) / avgBlockerDepthView;
        float filterRadiusUV = penumbraRatio * lightRadiusUV * NEAR_PLANE / zReceiverView;

        // STEP 3: filtering
        return PCF_Filter(uv, zReceiverNdc, filterRadiusUV, shadowMap);
    }
}

float3 LightRadiance(in Light light, in float3 posWorld, in float3 normalWorld, in int index)
{
    // Directional light
    float3 lightVec = light.type & LIGHT_DIRECTIONAL
                      ? -light.direction
                      : light.position - posWorld;
        
    float lightDist = length(lightVec);
    lightVec /= lightDist;

    // Spot light
    float spotFator = light.type & LIGHT_SPOT
                     ? pow(max(-dot(lightVec, light.direction), 0.0f), light.spotPower)
                      : 1.0f;
        
    // Distance attenuation
    float att = saturate((light.fallOffEnd - lightDist)
                         / (light.fallOffEnd - light.fallOffStart));
    //float att = smoothstep(light.fallOffEnd, light.fallOffStart, lightDist);

    // Shadow map
    float shadowFactor = 1.0;
    if (light.type & LIGHT_SHADOW)
    {
        const float nearZ = 0.01; // 카메라 설정과 동일
        
        // 1. Project posWorld to light screen
        // light.viewProj 사용
        float4 lightScreen = mul(float4(posWorld, 1.0), light.viewProj);
        lightScreen.xyz /= lightScreen.w;
        
        // 2. 카메라(광원)에서 볼 때의 텍스춰 좌표 계산
        // [-1, 1]x[-1, 1] -> [0, 1]x[0, 1]
        // 주의: 텍스춰 좌표와 NDC는 y가 반대
        float2 lightTexcoord = float2(lightScreen.x, -lightScreen.y);
        lightTexcoord += 1.0;
        lightTexcoord *= 0.5;
        
        uint width, height, numMips;
        Texture2D shadowMap = texture[shadowDepthOnlyStartIndex + index];
        shadowMap.GetDimensions(0, width, height, numMips);
        
        // Texel size
        float dx = 5.0 / (float) width;
        //shadowFactor = PCF_Filter(lightTexcoord.xy, lightScreen.z - 0.001, dx, shadowMap);
        shadowFactor = PCSS(lightTexcoord, lightScreen.z - 0.01, shadowMap, light.invProj, light.radius);
    }

    float3 radiance = light.radiance * spotFator * att * shadowFactor;

    return radiance;
}

float3 LightRadiance(in Light light, in float3 posWorld, in float3 normalWorld)
{
    // Directional light
    float3 lightVec = light.type & LIGHT_DIRECTIONAL
                      ? -light.direction
                      : light.position - posWorld;
        
    float lightDist = length(lightVec);
    lightVec /= lightDist;

    // Spot light
    float spotFator = light.type & LIGHT_SPOT
                     ? pow(max(-dot(lightVec, light.direction), 0.0f), light.spotPower)
                      : 1.0f;
        
    // Distance attenuation
    float att = saturate((light.fallOffEnd - lightDist)
                         / (light.fallOffEnd - light.fallOffStart));

    // Shadow map
    float shadowFactor = 1.0;
    float3 radiance = light.radiance * spotFator * att * shadowFactor;

    return radiance;
}

float4 main(PSInput input) : SV_TARGET
{
    float3 pixelToEye = normalize(eyeWorld - input.posWorld);
    float3 normalWorld = GetNormal(input);
    
    float3 albedo = useAlbedoMap ? texture[albedoIndex].SampleLevel(linearWrapSampler, input.texcoord, meshLodBias).rgb * albedoFactor
                                 : albedoFactor;
    float ao = useAOMap ? texture[aoIndex].SampleLevel(linearWrapSampler, input.texcoord, meshLodBias).r : 1.0;
    float metallic = useMetallicMap ? texture[metallicIndex].SampleLevel(linearWrapSampler, input.texcoord, meshLodBias).b * metallicFactor
                                    : metallicFactor;
    float roughness = useRoughnessMap ? texture[roughnessIndex].SampleLevel(linearWrapSampler, input.texcoord, meshLodBias).g * roughnessFactor
                                      : roughnessFactor;
    float3 emission = useEmissiveMap ? texture[emissiveIndex].SampleLevel(linearWrapSampler, input.texcoord, meshLodBias).rgb 
                                     : emissionFactor;
    
    float3 ambientLighting = AmbientLightingByIBL(albedo, normalWorld, pixelToEye, ao,
                                                  metallic, roughness) * strengthIBL;
    
    float3 directLighting = float3(0, 0, 0);
    // 포인트 라이트만 먼저 구현
    [unroll]
    for (int i = 0; i < MAX_LIGHTS; ++i)
    {
        if (light[i].type)
        {
            float3 lightVec = light[i].position - input.posWorld;
            float lightDist = length(lightVec);
            lightVec /= lightDist;
            float3 halfway = normalize(pixelToEye + lightVec);
        
            float NdotI = max(0.0, dot(normalWorld, lightVec));
            float NdotH = max(0.0, dot(normalWorld, halfway));
            float NdotO = max(0.0, dot(normalWorld, pixelToEye));
        
            const float3 Fdielectric = 0.04; // 비금속(Dielectric) 재질의 F0
            float3 F0 = lerp(Fdielectric, albedo, metallic);
            float3 F = SchlickFresnel(F0, max(0.0, dot(halfway, pixelToEye)));
            float3 kd = lerp(float3(1, 1, 1) - F, float3(0, 0, 0), metallic);
            float3 diffuseBRDF = kd * albedo;

            float D = NdfGGX(NdotH, roughness);
            float3 G = SchlickGGX(NdotI, NdotO, roughness);
            float3 specularBRDF = (F * D * G) / max(1e-5, 4.0 * NdotI * NdotO);
        
            float3 radiance = 0.0f;
            
            radiance = LightRadiance(light[i], input.posWorld, normalWorld, i);

            directLighting += (diffuseBRDF + specularBRDF) * radiance * NdotI;
        }
    }
    
    float4 pixelColor = float4(ambientLighting + directLighting + emission, 1.0);
    pixelColor = clamp(pixelColor, 0.0, 1000.0);
    
    return pixelColor;

}
