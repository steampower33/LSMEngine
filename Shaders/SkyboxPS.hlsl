#include "Common.hlsli"

TextureCube skyboxTexture[10] : register(t10, space0);
SamplerState g_sampler : register(s0, space0);

float4 main(PSInput input) : SV_TARGET
{
    return skyboxTexture[cubemapEnvIndex - 10].Sample(g_sampler, input.posWorld);
}
