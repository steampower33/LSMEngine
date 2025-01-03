#include "Common.hlsli"

PSInput main(VSInput input)
{
    PSInput result;
    
    float4 worldPosition = mul(float4(input.position, 1.0f), world);
    float4 viewPosition = mul(worldPosition, view);
    result.position = mul(viewPosition, proj);
    
    result.texcoord = input.texcoord;

    return result;
}