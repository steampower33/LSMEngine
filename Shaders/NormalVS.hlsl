#include "Common.hlsli"

struct GSInput
{
    float4 posModel : POSITION;
    float4 normalModel : NORMAL;
};

GSInput main(VSInput input)
{
    GSInput output;
    
    output.posModel = float4(input.posModel, 1.0);
    output.normalModel = float4(input.normalModel, 0.0);

    return output;
}
