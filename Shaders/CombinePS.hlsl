#include "Common.hlsli"

Texture2D g_texture[6] : register(t0, space1);
SamplerState g_sampler : register(s0, space0);

struct SamplingPSInput
{
    float4 posModel : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

float4 main(SamplingPSInput input) : SV_TARGET
{
    float4 color1 = g_texture[0].Sample(g_sampler, input.texcoord);
    float4 color2 = g_texture[1].Sample(g_sampler, input.texcoord);
    
    return color1 + color2 * strength;

}
