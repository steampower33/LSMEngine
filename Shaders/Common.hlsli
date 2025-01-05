struct VSInput
{
    float3 posModel : POSITION;
    float2 texcoord : TEXCOORD;
    float3 normalModel : NORMAL;
};

struct PSInput
{
    float4 posProj : SV_POSITION;
    float3 posWorld : POSITION;
    float3 normalWorld : NORMAL;
    float2 texcoord : TEXCOORD;
};

cbuffer GlobalConstants : register(b0)
{
    float4x4 view;
    float4x4 proj;
    float4x4 d1;
    float4x4 d2;
}

cbuffer MeshConstants : register(b1)
{
    float4x4 world;
    float4x4 worldIT;
    float4x4 d3;
    float4x4 d4;
}

cbuffer TextureIndexConstants : register(b2)
{
    uint diffuseIndex;
    uint cubemapIndex;
    uint dummy[14];
    float4x4 d5;
    float4x4 d6;
}