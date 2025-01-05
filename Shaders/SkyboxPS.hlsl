#include "Common.hlsli"

TextureCube skyboxTexture[] : register(t0, space0);
SamplerState g_sampler : register(s0, space0);

float4 main(PSInput input) : SV_TARGET
{
    return skyboxTexture[cubemapIndex].Sample(g_sampler, input.posWorld);
}
