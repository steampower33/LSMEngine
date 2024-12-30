#include "Common.hlsli"

cbuffer MeshConstants : register(b1)
{
    float4x4 world;
    float4x4 worldIT;
    float4x4 d3;
    float4x4 d4;
}

PSInput main(VSInput input)
{
    PSInput result;
    
    float4 worldPosition = mul(float4(input.position, 1.0f), world);
    float4 viewPosition = mul(worldPosition, view);
    result.position = mul(viewPosition, proj);
    
    result.texcoord = input.texcoord;

    return result;
}