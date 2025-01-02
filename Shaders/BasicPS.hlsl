#include "Common.hlsli"

Texture2D g_texture[] : register(t0, space0);
SamplerState g_sampler : register(s0, space0);

float4 main(PSInput input) : SV_TARGET
{
    return g_texture[1].Sample(g_sampler, input.texcoord);
}
