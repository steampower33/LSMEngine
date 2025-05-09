#define GROUP_SIZE_X 16
#define GROUP_SIZE_Y 16

Texture2D<float4> NormalTexture : register(t4);

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
    
    float3 eyeWorld;
    float p1;
};

[numthreads(GROUP_SIZE_X, GROUP_SIZE_Y, 1)]
void main(uint3 gid : SV_GroupID,
    uint3 gtid : SV_GroupThreadID,
    uint3 dtid : SV_DispatchThreadID)
{

}