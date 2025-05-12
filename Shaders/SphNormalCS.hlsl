
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
    float p1;

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

float3 ReconstructPosition(int2 pix, float z) {
    float2 invScreen = float2(invWidth, invHeight);

    float2 uv = (float2(pix) + 0.5) * invScreen;
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

    float dC = SmoothedDepthMap.Load(int3(pix, 0));
    if (dC >= 100.0 || dC <= 0.0)
    {
        NormalMap[pix] = float4(0.5f, 0.5f, 0.5f, 1.0f);
        LastScene[pix] = float4(0.0, 0.0, 0.0, 1.0);
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

    float3 normalVS = normalize(cross(ddx, ddy));
    float4 outputNormal = float4(normalVS * 0.5 + 0.5, 1.0);

    NormalMap[pix] = outputNormal;

    float3 worldPos = mul(float4(posC, 1.0), invView).xyz;

    float3 L = normalize(lightPos - worldPos);
    float3 V = normalize(eyeWorld - worldPos);
    float3 N = normalize(mul(normalVS, (float3x3)invView).xyz);
    float3 H = normalize(L + V);

    float  NdotL = saturate(dot(N, L));
    float  NdotH = saturate(dot(N, H));

    float3 ambient = ambientColor;
    float3 diffuse = diffuseColor * NdotL;
    float3 specular = specularColor * pow(NdotH, shininess);

    float  thickness = ThicknessMap.Load(int3(pix, 0)).r;
    float3 beerTrans = exp(-waterDensity * thickness * (1.0 - diffuseColor));
    float3 colorNoSpec = (ambient + diffuse) * beerTrans;

    float fresnel = saturate(fresnel0 + (1 - fresnel0) * pow(1 - saturate(dot(N, V)), fresnelPower));
    fresnel = clamp(fresnel, 0, fresnelClamp);

    float4 finalColor = float4(lerp(colorNoSpec, colorNoSpec + specular, fresnel), 1.0);

    LastScene[pix] = finalColor;
}