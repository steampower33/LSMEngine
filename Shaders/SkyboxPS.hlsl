#include "Common.hlsli"

TextureCube skyboxTexture[10] : register(t100, space0);

static const uint textureSizeOffset = 100;

float4 main(PSInput input) : SV_TARGET
{
    return skyboxTexture[cubemapEnvIndex - textureSizeOffset + choiceEnvMap].SampleLevel(linearWrapSampler, input.posWorld, envLodBias) * strengthIBL;
}
