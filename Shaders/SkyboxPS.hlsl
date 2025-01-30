#include "Common.hlsli"

TextureCube skyboxTexture[10] : register(t50, space0);

static const uint textureSizeOffset = 50;

float4 main(PSInput input) : SV_TARGET
{
    return skyboxTexture[cubemapEnvIndex - textureSizeOffset + choiceEnvMap].SampleLevel(linearWrapSampler, input.posWorld, envLodBias) * strengthIBL;
}
