
#define GROUP_SIZE_X 16
#define GROUP_SIZE_Y 16

Texture2D<float> ThicknessMap : register(t1);
Texture2D<float> SmoothedDepthMap : register(t3);
Texture2D<float4> Background : register(t6);

TextureCube<float4> DiffuseEnvMap : register(t8);
TextureCube<float4> SpecularEnvMap : register(t9);

RWTexture2D<float4> NormalMap : register(u2);
RWTexture2D<float4> Shaded : register(u4);

SamplerState SamplerLinearClamp : register(s0);
SamplerState SamplerLinearWrap : register(s1);

#define IOR_AIR 1.0
#define IOR_WATER 1.333
#define FRESNEL_0 0.02
#define FRESNEL_POWER 5.0

cbuffer ComputeParams : register(b1)
{
    float4x4 invProj;
    float4x4 invView;

    int   filterRadius;
    float sigmaSpatial;
    float sigmaDepth;
    float opacityMultiplier;

    uint width;
    float invWidth;
    uint height;
    float invHeight;

    float3 eyeWorld;
    float refractionStrength;

    float3 specularColor;
    float waterDensity;

    float fresnelClamp;
    float p0;
    float p1;
    float p2;
    
    float4 p3;
    float4 p4;
    float4 p5;
};

float3 LinearToneMapping(float3 color)
{
    float3 invGamma = float3(1, 1, 1) / 2.2;

    color = clamp(1.0 * color, 0., 1.);
    color = pow(color, invGamma);
    return color;
}

float3 ReconstructPosition(int2 pix, float z) {
    float2 invScreen = float2(invWidth, invHeight);

    float2 uv = (float2(pix)+0.5) * invScreen;
    float2 ndc = uv * 2.0 - 1.0;

    float4 clipPos = float4(ndc, 1.0, 1.0);

    float4 viewPosHom = mul(clipPos, invProj);
    float3 viewRay = viewPosHom.xyz / viewPosHom.w;

    return viewRay * (z / viewRay.z);
}

[numthreads(GROUP_SIZE_X, GROUP_SIZE_Y, 1)]
void main(uint3 gid : SV_GroupID,
    uint3 gtid : SV_GroupThreadID,
    uint3 dtid : SV_DispatchThreadID)
{
    int2 pix = dtid.xy;
    if (pix.x >= width || pix.y >= height) return;

    float2 uv = (float2(pix)+0.5) * float2(invWidth, invHeight);

    float dC = SmoothedDepthMap.Load(int3(pix, 0));
    if (dC >= 100.0 || dC <= 0.0)
    {
        NormalMap[pix] = float4(0.5f, 0.5f, 0.5f, 1.0f);
        Shaded[pix] = float4(LinearToneMapping(Background.SampleLevel(SamplerLinearClamp, uv, 0).rgb), 1.0);
        return;
    }

    // 이웃 좌표는 화면 밖으로 나가지 않도록 clamp
    int2 pixL = clamp(pix + int2(-1, 0), int2(0, 0), int2(width - 1, height - 1));
    int2 pixR = clamp(pix + int2(1, 0), int2(0, 0), int2(width - 1, height - 1));
    int2 pixU = clamp(pix + int2(0, -1), int2(0, 0), int2(width - 1, height - 1));
    int2 pixD = clamp(pix + int2(0, 1), int2(0, 0), int2(width - 1, height - 1));

    // 뎁스 읽어서 3D 위치 재구성
    float3 posL = ReconstructPosition(pixL, SmoothedDepthMap.Load(int3(pixL, 0)));
    float3 posR = ReconstructPosition(pixR, SmoothedDepthMap.Load(int3(pixR, 0)));
    float3 posU = ReconstructPosition(pixU, SmoothedDepthMap.Load(int3(pixU, 0)));
    float3 posD = ReconstructPosition(pixD, SmoothedDepthMap.Load(int3(pixD, 0)));
    float3 posC = ReconstructPosition(pix, SmoothedDepthMap.Load(int3(pix, 0)));

    // 중앙 차분: 양쪽 차분을 합쳐서 진짜 기울기 계산
    float3 ddx = (posR - posL) * 0.5;
    float3 ddy = (posD - posU) * 0.5;

    float3 viewNormal = normalize(cross(ddx, ddy));

    float4 outputNormal = float4(viewNormal * 0.5 + 0.5, 1.0);
    NormalMap[pix] = outputNormal;

    // 쉐이딩 -> 월드 공간
    float3 worldNormal = normalize(mul(viewNormal, (float3x3)invView));
    float3 worldPos = mul(float4(posC, 1.0), invView).xyz;
    float3 V = normalize(eyeWorld - worldPos);

    float2 latlongUV;
    latlongUV.x = atan2(worldNormal.z, worldNormal.x) * (1.0 / 6.2831853) + 0.5;
    latlongUV.y = asin(worldNormal.y) * (1.0 / 3.1415926) + 0.5;

    float3 diffuseEnvColor = DiffuseEnvMap.SampleLevel(SamplerLinearWrap, float3(latlongUV, 0.0), 0.0).rgb;

    float3 reflectionDir = reflect(-V, worldNormal);
    float3 specularEnvColor = SpecularEnvMap.SampleLevel(SamplerLinearWrap, reflectionDir, 0.0).rgb;

    specularEnvColor *= specularColor;

    // Thickness
    float  thickness = ThicknessMap.Load(int3(pix, 0)).r;

    float3 beerTrans = exp(-waterDensity * thickness * (1.0 - diffuseEnvColor));

    // 굴절 효과 적용
    float3 rayDirView = normalize(posC);
    float3 refractionDir = refract(rayDirView, viewNormal, IOR_AIR / IOR_WATER);

    float2 refracted_uv = uv + refractionDir.xy * thickness * refractionStrength;
    refracted_uv = clamp(refracted_uv, float2(0.0, 0.0), float2(1.0, 1.0));
    float3 refracted_background_color = Background.SampleLevel(SamplerLinearClamp, refracted_uv, 0).rgb;
    float3 transmitted_background_color = refracted_background_color * beerTrans;

    //float3 transmitted_color = diffuseEnvColor + (diffuseColor * beerTrans) + transmitted_background_color;
    float3 transmitted_color = transmitted_background_color;

    float3 reflected_color = specularEnvColor;

    float fresnel = clamp(FRESNEL_0 + (1 - FRESNEL_0) * pow(1 - saturate(dot(worldNormal, V)), FRESNEL_POWER), 0.0, fresnelClamp);

    float3 final_rgb = lerp(transmitted_color, reflected_color, fresnel);

    float opacity = saturate(thickness * opacityMultiplier);

    Shaded[pix] = float4(LinearToneMapping(final_rgb), opacity);
    //Shaded[pix] = float4(ambient + diffuse + specular, 1.0);
    //float dNorm = (dC - 0.1) / (10.0 - 0.1);
    //Shaded[pix] = float4(dNorm, dNorm, dNorm, 1.0);;
}