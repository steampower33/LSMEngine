#include "Common.hlsli"

PSInput main( VSInput input )
{
    PSInput output;
    
    float4 pos = mul(float4(input.posModel, 1.0), world);
    pos = mul(pos, view);
    output.posProj = mul(pos, proj);

    return output;
}