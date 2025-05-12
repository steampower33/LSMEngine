
#define GROUP_SIZE_X 16
#define GROUP_SIZE_Y 16

Texture2D<float> ThicknessMap : register(t1);
Texture2D<float> SmoothedDepthMap : register(t3);

RWTexture2D<float4> NormalMap : register(u2);
RWTexture2D<float4> LastScene : register(u3);

#define IOR_AIR 1.0
#define IOR_WATER 1.333

cbuffer ComputeParams : register(b1)
{
    float4x4 invProj;
    float4x4 invView;

    int   filterRadius;
    float sigmaSpatial;
    float sigmaDepth;
    float shininess;

    uint width;
    float invWidth;
    uint height;
    float invHeight;

    float3 eyeWorld;
    float refractionStrength;

    float3 lightPos;
    float p;

    float3 lightColor;
    float waterDensity;

    float3 ambientColor;
    float fresnel0;

    float3 diffuseColor;
    float fresnelPower;

    float3 specularColor;
    float fresnelClamp;
};

float3 LinearToneMapping(float3 color)
{
    float3 invGamma = float3(1, 1, 1) / 2.2;

    color = clamp(1.0 * color, 0., 1.);
    color = pow(color, invGamma);
    return color;
}

float3 ReconstructPosition(int2 p, float z) {
    float2 invScreen = float2(invWidth, invHeight);

    float2 uv = (float2(p) + 0.5) * invScreen;
    float4 clip = float4(uv * 2.0 - 1.0, 0.0, -1.0);
    float3 vpos = normalize(mul(invProj, clip).xyz);
    return vpos * z;
}

[numthreads(GROUP_SIZE_X, GROUP_SIZE_Y, 1)]
void main(uint3 gid : SV_GroupID,
    uint3 gtid : SV_GroupThreadID,
    uint3 dtid : SV_DispatchThreadID)
{
    int2 pix = dtid.xy;
    if (pix.x >= width || pix.y >= height) return;

    float dC = SmoothedDepthMap.Load(int3(pix, 0));
    if (dC >= 100.0 || dC <= 0.0)
    {
        NormalMap[pix] = float4(0.0, 0.0, 0.0, 1.0f);
        LastScene[pix] = float4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    int2 pixR = pix + int2(1, 0);
    int2 pixU = pix + int2(0, -1);

    if (pixR.x >= width || SmoothedDepthMap.Load(int3(pixR, 0)) >= 100.0f ||
        pixU.y >= height || SmoothedDepthMap.Load(int3(pixU, 0)) >= 100.0f)
    {
        NormalMap[pix] = float4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    float dR = SmoothedDepthMap.Load(int3(pixR, 0));
    float dU = SmoothedDepthMap.Load(int3(pixU, 0));

    float3 posC = ReconstructPosition(pix, dC);
    float3 posR = ReconstructPosition(pixR, dR);
    float3 posU = ReconstructPosition(pixU, dU);

    float3 ddx = posR - posC;
    float3 ddy = posU - posC;

    float3 nView = normalize(cross(ddy, ddx));

    float3x3 R = (float3x3)invView;
    float3 nWorld = normalize(mul(R, nView)).xyz;

    float3 nColor = nView * 0.5 + 0.5;

    NormalMap[pix] = float4(nColor, 1.0);

    // Lighting
    float3 worldPos = mul(invView, float4(posC, 1.0)).xyz;

    float3 L = normalize(lightPos - worldPos);
    float3 V = normalize(eyeWorld - worldPos);
    float3 H = normalize(L + V);

    float NdotL = max(dot(nWorld, L), 0.0);
    float NdotH = saturate(dot(nWorld, H));

    float3 diff = diffuseColor * NdotL;

    float3 spec = specularColor * pow(NdotH, shininess);

    float4 finalColor = float4(ambientColor + diff + spec, 1.0);

    //// 흡수, 투과
    //float thickness = ThicknessMap.Load(int3(pix, 0));

    //float3 transmittance = exp(-waterDensity * thickness * (1.0 - waterColor));

    //float cosTheta = saturate(dot(nWorld, V)); // Normal과 View Direction의 내적 (월드 공간)
    //float fresnel = clamp(fresnel0 + (1.0 - fresnel0) * pow(1.0 - cosTheta, fresnelPower), 0.0, fresnelClamp);

    //float3 lighting = waterColor * NdotL + specular * specularIntensity; // Diffuse + Specular (Diffuse Color는 waterColor로 사용)
    //float3 absorbedLighting = lighting * transmittance; // 라이팅된 표면색에 흡수 적용
    //float3 testReflectionColor = float3(0.1f, 0.1f, 0.1f); // 흰색 반사 테스트
    //float3 finalColor = lerp(absorbedLighting, testReflectionColor, fresnel); // 흡수된 색과 반사 테스트색을 프레넬로 혼합
    //finalColor += ambient; // Ambient 더하기

    LastScene[pix] = finalColor;
}