#include "Common.hlsli"

Texture2D g_texture[10] : register(t0, space0);
TextureCube skyboxTexture[10] : register(t10, space0);
SamplerState g_sampler : register(s0, space0);

float4 main(PSInput input) : SV_TARGET
{
    float3 toEye = normalize(eyeWorld - input.posWorld);
    
    float3 color = float3(0.0, 0.0, 0.0);
    
    int i = 0;
    
    float3 normalWorld = input.normalWorld;
    
    if (isUseNormalMap && normalIndex != 0)
    {
        float3 normalTex = g_texture[normalIndex].Sample(g_sampler, input.texcoord).rgb;
        normalTex = 2.0 * normalTex - 1.0; // 범위 조절 [-1.0, 1.0]
        
        float3 N = normalWorld;
        float3 T = normalize(input.tangentWorld - dot(N, input.tangentWorld) * N);
        float3 B = cross(N, T);

        float3x3 TBN = float3x3(T, B, N);
        normalWorld = normalize(mul(normalTex, TBN));

    }
    
    [unroll]
    for (i = 0; i < NUM_DIR_LIGHTS; ++i)
    {
        color += ComputeDirectionalLight(lights[i], material, normalWorld, toEye);
    }
    
    [unroll]
    for (i = NUM_DIR_LIGHTS; i < NUM_DIR_LIGHTS + NUM_POINT_LIGHTS; ++i)
    {
        color += ComputePointLight(lights[i], material, input.posWorld, normalWorld, toEye);
    }
    
    [unroll]
    for (i = NUM_DIR_LIGHTS + NUM_POINT_LIGHTS; i < NUM_DIR_LIGHTS + NUM_POINT_LIGHTS + NUM_SPOT_LIGHTS; ++i)
    {
        color += ComputeSpotLight(lights[i], material, input.posWorld, normalWorld, toEye);
    }
    
    float4 diffuse = skyboxTexture[cubemapDiffuseIndex - 10].Sample(g_sampler, normalWorld) + float4(color, 0.0);
    float4 specular = skyboxTexture[cubemapSpecularIndex - 10].Sample(g_sampler, reflect(-toEye, normalWorld));
    
    diffuse.xyz *= material.diffuse;
    
    specular *= pow((specular.x + specular.y + specular.z) / 3.0, material.shininess);
    specular.xyz *= material.specular;
    
    if (isUseTexture && albedoIndex != 0)
    {
        float dist = length(eyeWorld - input.posWorld);
        float distMin = 10.0;
        float distMax = 50.0;
        float lod = 10.0 * saturate(dist / (distMax - distMin));
        diffuse *= g_texture[albedoIndex].SampleLevel(g_sampler, input.texcoord, lod);
    }
    
    return diffuse + specular;
}
