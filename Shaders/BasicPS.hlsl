#include "Common.hlsli"

Texture2D g_texture[] : register(t0, space0);
SamplerState g_sampler : register(s0, space0);

float4 main(PSInput input) : SV_TARGET
{
    float3 toEye = normalize(eyeWorld - input.posWorld);
    
    float3 color = float3(0.0, 0.0, 0.0);
    
    int i = 0;
    
    [unroll]
    for (i = 0; i < NUM_DIR_LIGHTS; ++i)
    {
        color += ComputeDirectionalLight(lights[i], material, input.normalWorld, toEye);
    }
    
    return isUseTexture ?
    float4(color, 1.0) * g_texture[diffuseIndex].Sample(g_sampler, input.texcoord) :
    float4(lights[0].strength.x, lights[0].strength.y, lights[0].strength.z, 1.0);
}
