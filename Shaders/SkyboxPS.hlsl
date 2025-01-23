#include "Common.hlsli"

TextureCube skyboxTexture[10] : register(t50, space0);
SamplerState wrapSampler : register(s0, space0);

static const uint textureSizeOffset = 50;

float4 main(PSInput input) : SV_TARGET
{
    return skyboxTexture[cubemapEnvIndex - textureSizeOffset + choiceEnvMap].SampleLevel(wrapSampler, input.posWorld, envLodBias) * strengthIBL;
}
