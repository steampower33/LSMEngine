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
    float dist = length(eyeWorld - input.posWorld);
    float distMin = 10.0;
    float distMax = 50.0;
    float lod = 10.0 * saturate(dist / (distMax - distMin));
    return g_texture[albedoIndex].SampleLevel(g_sampler, input.texcoord, lod);
    
}
