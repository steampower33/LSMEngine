#include "Common.hlsli"

Texture2D g_texture[5] : register(t0, space1);
SamplerState g_sampler : register(s0, space0);

static const float weights[5] = { 0.0545, 0.2442, 0.4026, 0.2442, 0.0545 };

struct SamplingPSInput
{
    float4 posModel : SV_POSITION;
    float2 texcoord : TEXCOORD;
};


float4 main(SamplingPSInput input) : SV_TARGET
{
    // Compute Shader X
    float3 color = (0.0, 0.0, 0.0);
    for (int i = -2; i < 3; i++)
    {
        color += g_texture[index].Sample(g_sampler, input.texcoord + float2(dx * i, 0.0)).rgb * weights[i + 2];
    }
    return float4(color, 1.0);
}