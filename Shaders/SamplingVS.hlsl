#include "Common.hlsli"

struct SamplingPSInput
{
    float4 posModel : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

SamplingPSInput main(VSInput input)
{
    SamplingPSInput output;
        
    output.posModel = float4(input.posModel, 1.0);
    output.texcoord = input.texcoord;
    
    return output;
}