#include "Common.hlsli"

Texture2D g_texture[] : register(t0, space0);
SamplerState g_sampler : register(s0, space0);

float4 main(PSInput input) : SV_TARGET
{
    return float4(1.0, 1.0, 1.0, 1.0) * g_texture[diffuseIndex].Sample(g_sampler, input.texcoord);
}
