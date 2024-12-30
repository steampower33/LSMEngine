#include "Common.hlsli"

PSInput main( VSInput input )
{
    PSInput output;
    output.position = float4(input.position, 1.0);
    output.texcoord = input.texcoord;
    return output;
}