#include "Common.hlsli"

Texture2D g_texture[2] : register(t0, space1);
SamplerState g_sampler : register(s0, space0);

struct SamplingPSInput
{
    float4 posModel : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

float4 main(SamplingPSInput input) : SV_TARGET
{
    float4 color = g_texture[0].Sample(g_sampler, input.texcoord);
    
    return (color.r + color.g + color.b) / 3 < threshold ? 
    float4(0.0, 0.0, 0.0, 1.0) : color;
}
