#define GROUP_SIZE_X 16
#define GROUP_SIZE_Y 16

Texture2D<float> SmoothedDepth : register(t3);

RWTexture2D<float4> NormalTexture : register(u1);
//RWTexture2D<float4> SceneTexture : register(u2);

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
};

float3 Reconstruct(int2 p, float z) {
    float2 invScreen = float2(invWidth, invHeight);

    float2 uv = (float2(p)+0.5f) * invScreen;
    float4 ndc = float4(uv * 2 - 1, z, 1);
    float4 vpos = mul(invProj, ndc);
    return (vpos.xyz / vpos.w);
}

[numthreads(GROUP_SIZE_X, GROUP_SIZE_Y, 1)]
void main(uint3 gid : SV_GroupID,
    uint3 gtid : SV_GroupThreadID,
    uint3 dtid : SV_DispatchThreadID)
{
    if (dtid.x >= width || dtid.y >= height)
        return;

    uint2 pix = dtid.xy;

    float dC = SmoothedDepth.Load(int3(pix, 0));               // center
    float dR = SmoothedDepth.Load(int3(pix + int2(1, 0), 0));  // right
    float dL = SmoothedDepth.Load(int3(pix - int2(1, 0), 0));  // left
    float dU = SmoothedDepth.Load(int3(pix + int2(0, 1), 0));  // up
    float dD = SmoothedDepth.Load(int3(pix - int2(0, 1), 0));  // down

    float3 P = Reconstruct(pix, dC);
    float3 PR = Reconstruct(pix + int2(1, 0), dR);
    float3 PL = Reconstruct(pix - int2(1, 0), dL);
    float3 PU = Reconstruct(pix + int2(0, 1), dU);
    float3 PD = Reconstruct(pix - int2(0, 1), dD);

    float3 dx = PR - PL;
    float3 dy = PU - PD;
    float3 N = normalize(cross(dx, dy));

    N = N * 0.5f + 0.5f;

    NormalTexture[dtid.xy] = float4(N, 1);
    //SceneTexture[dtid.xy] = float4(N, 1);
}