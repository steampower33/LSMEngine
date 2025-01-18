#include "Common.hlsli"

Texture2D g_texture[10] : register(t0, space0);
TextureCube skyboxTexture[10] : register(t10, space0);
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
    
    float4 diffuse = skyboxTexture[cubemapDiffuseIndex - 10].Sample(g_sampler, input.normalWorld);
    float4 specular = skyboxTexture[cubemapSpecularIndex - 10].Sample(g_sampler, reflect(-toEye, input.normalWorld));
    
    specular *= pow((specular.x + specular.y + specular.z) / 3.0, material.shininess);
    
    diffuse.xyz *= material.diffuse;
    specular.xyz *= material.specular;
    
    if (isUseTexture)
    {
        float dist = length(eyeWorld - input.posWorld);
        float distMin = 10.0;
        float distMax = 50.0;
        float lod = 10.0 * saturate(dist / (distMax - distMin));
        diffuse *= g_texture[colorIndex].SampleLevel(g_sampler, input.texcoord, lod);
    }
    
    return diffuse + specular;
}
