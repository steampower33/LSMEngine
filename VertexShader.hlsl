
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

struct VertexShaderInput
{
    float3 position : POSITION;
    float2 texcoord : TEXCOORD;
    float3 normal : NORMAL;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

PSInput main(VertexShaderInput input)
{
    PSInput result;
    
    float4 worldPosition = mul(float4(input.position, 1.0f), world);
    float4 viewPosition = mul(worldPosition, view);
    result.position = mul(viewPosition, proj);
    
    result.texcoord = input.texcoord;

    return result;
}