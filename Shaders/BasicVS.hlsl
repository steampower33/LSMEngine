#include "Common.hlsli"

Texture2D g_texture[10] : register(t0, space0);
SamplerState g_sampler : register(s0, space0);

PSInput main(VSInput input)
{
    PSInput output;
    
    float4 normal = float4(input.normalModel, 0.0f);
    output.normalWorld = mul(normal, worldIT).xyz;
    output.normalWorld = normalize(output.normalWorld);
    
    float4 tangentWorld = float4(input.tangentModel, 0.0f);
    tangentWorld = mul(tangentWorld, world);
    
    float4 pos = float4(input.posModel, 1.0f);
    pos = mul(pos, world);
    
    if (isUseHeightMap && heightIndex != 0)
    {
        float height = g_texture[heightIndex].SampleLevel(g_sampler, input.texcoord, 0).r;
        height = 2.0 * height - 1.0;
        
        pos += float4(output.normalWorld * height * heightScale, 0.0);
    }
    
    output.posWorld = pos.xyz;
    
    pos = mul(pos, view);
    pos = mul(pos, proj);
    
    output.posProj = pos;
    output.texcoord = input.texcoord;
    output.tangentWorld = tangentWorld.xyz;
    
    return output;
}