#define GROUP_SIZE_X 16
#define GROUP_SIZE_Y 16

Texture2D<float> SmoothedDepth : register(t3);

RWTexture2D<float4> NormalTexture : register(u1);
RWTexture2D<float4> SceneTexture : register(u2);

// Index of refraction for water
#define IOR 1.333

// Ratios of air and water IOR for refraction
// Air to water
#define ETA 1.0/IOR

// Fresnel at 0¡Æ
#define F0 0.02

cbuffer RenderParams : register(b0)
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
    float p1;

    float3 lightPos;
    float p2;

    float3 ambient;
    float p3;

    float3 diffuse;
    float p4;

    float3 specular;
    float p5;

    float4 p6;
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
    uv.y = 1.0 - uv.y;
    float4 clip = float4(uv * 2.0 - 1.0, 0, 1.0);
    float4 vpos = mul(invProj, clip);
    return vpos.xyz / vpos.w * z;
}

[numthreads(GROUP_SIZE_X, GROUP_SIZE_Y, 1)]
void main(uint3 gid : SV_GroupID,
    uint3 gtid : SV_GroupThreadID,
    uint3 dtid : SV_DispatchThreadID)
{
    int2 pix = dtid.xy;
    if (pix.x >= width || pix.y >= height) return;

    float dC = SmoothedDepth.Load(int3(pix, 0));
    if (dC >= 100.0 || dC <= 0.0)
    {
        NormalTexture[pix] = float4(0.5f, 0.5f, 1.0f, 1.0f);
        SceneTexture[pix] = float4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    int2 pixR = pix + int2(1, 0);
    int2 pixU = pix + int2(0, 1);

    if (pixR.x >= width || SmoothedDepth.Load(int3(pixR, 0)) >= 100.0f ||
        pixU.y >= height || SmoothedDepth.Load(int3(pixU, 0)) >= 100.0f)
    {
        NormalTexture[pix] = float4(0.5f, 0.5f, 0.5f, 1.0f);
        return;
    }

    float dR = SmoothedDepth.Load(int3(pixR, 0));
    float dU = SmoothedDepth.Load(int3(pixU, 0));

    float3 posC = ReconstructPosition(pix, dC);
    float3 posR = ReconstructPosition(pixR, dR);
    float3 posU = ReconstructPosition(pixU, dU);

    float3 ddx = posR - posC;
    float3 ddy = posU - posC;

    float3 n = normalize(cross(ddy, ddx));

    float3 nWorld = normalize(mul((float3x3)invView, n));

    float3 nColor = nWorld * 0.5 + 0.5;

    NormalTexture[pix] = float4(nColor, 1.0);

    float3 worldPos = mul((float3x3)invView, posC);

    float3 L = normalize(lightPos - worldPos);
    float3 V = normalize(eyeWorld - worldPos);
    float3 H = normalize(L + V);

    float diffuseIntensity = saturate(dot(nWorld, L));

    float specularIntensity = pow(saturate(dot(nWorld, H)), shininess);

    float3 finalColor = ambient
        + diffuse * diffuseIntensity
        + specular * specularIntensity;

    SceneTexture[pix] = float4(LinearToneMapping(finalColor), 1.0);
}