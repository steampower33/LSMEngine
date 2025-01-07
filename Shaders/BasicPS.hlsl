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
    
    for (i = NUM_DIR_LIGHTS; i < NUM_DIR_LIGHTS + NUM_POINT_LIGHTS; ++i)
    {
        color += ComputePointLight(lights[i], material, input.posWorld, input.normalWorld, toEye);
    }
    
    for (i = NUM_DIR_LIGHTS + NUM_POINT_LIGHTS; i < NUM_DIR_LIGHTS + NUM_POINT_LIGHTS + NUM_SPOT_LIGHTS; ++i)
    {
        color += ComputeSpotLight(lights[i], material, input.posWorld, input.normalWorld, toEye);
    }
    
    return isUseTexture ? float4(color, 1.0) * g_texture[diffuseIndex].Sample(g_sampler, input.texcoord) : float4(color, 1.0);

}
