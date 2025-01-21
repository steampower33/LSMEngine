#include "Common.hlsli"

TextureCube skyboxTexture[10] : register(t10, space0);
SamplerState wrapSampler : register(s0, space0);

static const uint textureSizeOffset = 10;

float4 main(PSInput input) : SV_TARGET
{
    return skyboxTexture[cubemapEnvIndex - textureSizeOffset].Sample(wrapSampler, input.posWorld);
}
