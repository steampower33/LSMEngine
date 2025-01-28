#include "Common.hlsli"

Texture2D texture[50] : register(t0, space0);
TextureCube skyboxTexture[10] : register(t50, space0);
SamplerState wrapSampler : register(s0, space0);
SamplerState clampSampler : register(s1, space0);

static const uint textureSizeOffset = 50;
static const float3 Fdielectric = 0.04; // 비금속(Dielectric) 재질의 F0

float3 SchlickFresnel(float3 F0, float NdotH)
{
    return F0 + (1.0 - F0) * pow(2.0, (-5.55473 * NdotH - 6.98316) * NdotH);
}

float3 GetNormal(PSInput input)
{
    float3 normalWorld = input.normalWorld;
    
    if (useNormalMap && normalIndex != 0) // NormalWorld를 교체
    {
        float3 normal = texture[normalIndex].SampleLevel(wrapSampler, input.texcoord, meshLodBias).rgb;
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
    float3 irradiance = skyboxTexture[cubemapDiffuseIndex - textureSizeOffset].SampleLevel(wrapSampler, normalWorld, 0.0).rgb;
    
    return kd * albedo * irradiance;
}

float3 SpecularIBL(float3 albedo, float3 normalWorld, float3 pixelToEye,
                   float metallic, float roughness)
{
    float2 specularBRDF = texture[brdfIndex].SampleLevel(clampSampler, float2(dot(normalWorld, pixelToEye), 1.0 - roughness), 0.0).rg;
    float3 specularIrradiance = skyboxTexture[cubemapSpecularIndex - textureSizeOffset].
        SampleLevel(wrapSampler, reflect(-pixelToEye, normalWorld), 2 + roughness * 5.0f).rgb;
    
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
    
    float3 albedo = useAlbedoMap ? texture[albedoIndex].SampleLevel(wrapSampler, input.texcoord, meshLodBias).rgb * albedoFactor
                                 : albedoFactor;
    float ao = useAOMap ? texture[aoIndex].SampleLevel(wrapSampler, input.texcoord, meshLodBias).r : 1.0;
    float metallic = useMetallicMap ? texture[metallicIndex].SampleLevel(wrapSampler, input.texcoord, meshLodBias).b * metallicFactor
                                    : metallicFactor;
    float roughness = useRoughnessMap ? texture[roughnessIndex].SampleLevel(wrapSampler, input.texcoord, meshLodBias).g * roughnessFactor
                                      : roughnessFactor;
    float3 emission = useEmissiveMap ? texture[emissiveIndex].SampleLevel(wrapSampler, input.texcoord, meshLodBias).rgb 
                                     : emissionFactor;
    
    float3 ambientLighting = AmbientLightingByIBL(albedo, normalWorld, pixelToEye, ao,
                                                  metallic, roughness) * strengthIBL;
    
    float3 directLighting = float3(0, 0, 0);
    // 포인트 라이트만 먼저 구현
    [unroll]
    for (int i = 0; i < MAX_LIGHTS; ++i)
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
            
        radiance = LightRadiance(light[i], input.posWorld, normalWorld);

        directLighting += (diffuseBRDF + specularBRDF) * radiance * NdotI;
    }
    
    float4 pixelColor = float4(ambientLighting + directLighting + emission, 1.0);
    pixelColor = clamp(pixelColor, 0.0, 1000.0);
    
    return pixelColor;

}
