
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
    float2 ndc = uv * 2.0 - 1.0;
    float4 clipPos = float4(ndc, 1.0, 1.0);

    float4 viewPosHom = mul(invProj, clipPos);
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

    int2 pixL = pix + int2(-1, 0);
    int2 pixR = pix + int2(1, 0);
    int2 pixD = pix + int2(0, 1);
    int2 pixU = pix + int2(0, -1);

    float dL = SmoothedDepthMap.Load(int3(pixL, 0));
    float dR = SmoothedDepthMap.Load(int3(pixR, 0));
    float dD = SmoothedDepthMap.Load(int3(pixD, 0));
    float dU = SmoothedDepthMap.Load(int3(pixU, 0));

    if (pixL.x < 0 || dL >= 100.0f ||
        pixR.x >= width || dR >= 100.0f ||
        pixD.y >= height || dD >= 100.0f ||
        pixU.y < 0 || dU >= 100.0f)
    {
        NormalMap[pix] = float4(0.5f, 0.5f, 0.5f, 1.0f);
        return;
    }

    float3 posC = ReconstructPosition(pix, dC);
    float3 posL = ReconstructPosition(pixL, dL);
    float3 posR = ReconstructPosition(pixR, dR);
    float3 posD = ReconstructPosition(pixD, dD);
    float3 posU = ReconstructPosition(pixU, dU);

    float3 ddx = posR - posC;
    float3 ddx2 = posC - posL;
    if (abs(ddx.z) > abs(ddx2.z))
        ddx = ddx2;

    float3 ddy = posU - posC;
    float3 ddy2 = posC - posD;
    if (abs(ddy.z) > abs(ddy2.z))
        ddy = ddy2;

    float3 normalVS = normalize(cross(ddx, ddy));
    normalVS.x *= -1;

    float4 outputNormal = float4(normalVS * 0.5 + 0.5, 1.0);

    NormalMap[pix] = outputNormal;

    float3 nWorld = mul((float3x3)invView, normalVS);

    // Lighting
    float4 worldPos_h = mul(invView, float4(posC, 1.0));
    float3 worldPos = worldPos_h.xyz / worldPos_h.w;

    float3 N = normalize(nWorld);

    float3 L = normalize(lightPos - worldPos);
    float3 V = normalize(eyeWorld - worldPos);
    float3 H = normalize(L + V);

    float NdotL = saturate(dot(nWorld, L));
    float NdotH = saturate(dot(nWorld, H));

    float3 ambient = ambientColor;
    float3 diffuse = diffuseColor * NdotL;
    float3 specular = specularColor * pow(NdotH, shininess);
    float3 localLighting = ambient + diffuse + specular;

    float thickness = ThicknessMap.Load(int3(pix, 0)).r;

    float3 beerTransmittance = exp(-waterDensity * thickness * (1.0 - ambientColor));

    float cosTheta = saturate(dot(N, V));
    float fresnel = fresnel0 + (1.0 - fresnel0) * pow(1.0 - cosTheta, fresnelPower);
    fresnel = clamp(fresnel, 0.0f, fresnelClamp);

    float3 absorbedLocalLighting = localLighting * beerTransmittance;

    float3 testReflectionColor = specularColor;

    float3 finalColor_rgb = lerp(absorbedLocalLighting, testReflectionColor, fresnel);

    LastScene[pix] = float4(finalColor_rgb, 1.0);
}