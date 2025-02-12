#include "Common.hlsli"

Texture2D g_texture : register(t0, space0);

struct SamplingPSInput
{
    float4 posModel : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

float4 main(SamplingPSInput input) : SV_TARGET
{
    float3 color = g_texture.Sample(linearWrapSampler, input.texcoord).xyz;
    
    float l = (color.x + color.y + color.y) / 3;
    
    if (l > 0.0)
    {
        return float4(color, 1.0f);
    }
    else
    {
        return float4(0.0f, 0.0f, 0.0f, 0.0f);
    }
}
