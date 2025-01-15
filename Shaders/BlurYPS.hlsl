#include "Common.hlsli"

Texture2D g_texture[] : register(t0, space1);
SamplerState g_sampler : register(s0, space0);

static const float weights[5] = { 0.0545f, 0.2442f, 0.4026f, 0.2442f, 0.0545f };

struct SamplingPSInput
{
    float4 posModel : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

float4 main(SamplingPSInput input) : SV_TARGET
{
    float3 color = float3(0.0, 0.0, 0.0);
    for (int i = -2; i < 3; i++)
    {
        color += g_texture[index].Sample(g_sampler, input.texcoord + float2(0.0, dy * i)).rgb * weights[i + 2];
    }
    return float4(color, 1.0);
}