#define GROUP_SIZE_X 16
#define GROUP_SIZE_Y 16

Texture2D<float> SmoothedDepth : register(t3);

RWTexture2D<float4> NormalTexture : register(u1);
RWTexture2D<float4> SceneTexture : register(u2);

cbuffer RenderParams : register(b0)
{
    int   filterRadius;
    float sigmaSpatial;
    float sigmaDepth;
    float p;

    uint width;
    float invWidth;
    uint height;
    float invHeight;

    float4x4 invProj;
    float4x4 invView;

    float3 eyeWorld;
    float p1;
};

float3 ReconstructPosition(int2 p, float z) {
    float2 invScreen = float2(invWidth, invHeight);

    float2 uv = (float2(p) + 0.5) * invScreen;
    float4 ndc = float4(uv * 2.0 - 1.0, 0.0, -1.0);
    float4 vpos = mul(invProj, ndc);
    return (vpos.xyz / vpos.w) * z;
}

[numthreads(GROUP_SIZE_X, GROUP_SIZE_Y, 1)]
void main(uint3 gid : SV_GroupID,
    uint3 gtid : SV_GroupThreadID,
    uint3 dtid : SV_DispatchThreadID)
{
    if (dtid.x >= width || dtid.y >= height)
        return;

    uint2 pix = dtid.xy;

    float dC = SmoothedDepth.Load(int3(pix, 0));
    if (dC == 0) return;

    float3 centerPos = ReconstructPosition(pix, dC);

    // dx 후보
    float3 dx1 = centerPos - ReconstructPosition(pix + int2(-1, 0), SmoothedDepth.Load(int3(pix + int2(-1, 0), 0)));
    float3 dx2 = ReconstructPosition(pix + int2(1, 0), SmoothedDepth.Load(int3(pix + int2(1, 0), 0))) - centerPos;
    float3 dx = (abs(dx1.z) < abs(dx2.z)) ? dx1 : dx2;

    // dy 후보
    float3 dy1 = centerPos - ReconstructPosition(pix + int2(0, -1), SmoothedDepth.Load(int3(pix + int2(0, -1), 0)));
    float3 dy2 = ReconstructPosition(pix + int2(0, 1), SmoothedDepth.Load(int3(pix + int2(0, 1), 0))) - centerPos;
    float3 dy = (abs(dy1.z) < abs(dy2.z)) ? dy1 : dy2;

    float3 N = normalize(cross(dx, dy));

    NormalTexture[dtid.xy] = float4(N * 0.5 + 0.5, 1);

    float3 lightDir = float3(0.0, 1.0, 1.0);
    float3 V = normalize(eyeWorld - mul(invView, float4(centerPos, 1)).xyz);
    float3 L = normalize(lightDir); // or from position
    float3 H = normalize(V + L);

    float shininess = 32.0;
    float diff = max(0, dot(N, L));
    float spec = pow(max(0, dot(N, H)), shininess);

    float3 color = diff * float3(1.0, 1.0, 1.0) + spec * float3(1.0, 1.0, 1.0);
    SceneTexture[pix] = float4(color, 1);
}