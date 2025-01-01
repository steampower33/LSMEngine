#include "Common.hlsli"

Texture2D g_texture[8] : register(t0);
SamplerState g_sampler : register(s0);

float4 main(PSInput input) : SV_TARGET
{
    return g_texture[1].Sample(g_sampler, input.texcoord);
}
