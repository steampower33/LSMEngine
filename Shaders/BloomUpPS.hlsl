#include "Common.hlsli"

Texture2D g_texture[] : register(t0, space1);
SamplerState clampSampler : register(s1, space0);

struct SamplingPSInput
{
    float4 posModel : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

float4 main(SamplingPSInput input) : SV_TARGET
{
    float x = input.texcoord.x;
    float y = input.texcoord.y;
    
    float3 a = g_texture[index].Sample(clampSampler, float2(x - dx, y + dy)).rgb;
    float3 b = g_texture[index].Sample(clampSampler, float2(x, y + dy)).rgb;
    float3 c = g_texture[index].Sample(clampSampler, float2(x + dx, y + dy)).rgb;
    
    float3 d = g_texture[index].Sample(clampSampler, float2(x - dx, y)).rgb;
    float3 e = g_texture[index].Sample(clampSampler, float2(x, y)).rgb;
    float3 f = g_texture[index].Sample(clampSampler, float2(x + dx, y)).rgb;
    
    float3 g = g_texture[index].Sample(clampSampler, float2(x - dx, y - dy)).rgb;
    float3 h = g_texture[index].Sample(clampSampler, float2(x, y - dy)).rgb;
    float3 i = g_texture[index].Sample(clampSampler, float2(x + dx, y - dy)).rgb;
    
    float3 color = e * 4.0;
    color += (b + d + f + h) * 2.0;
    color += (a + c + g + i);
    color *= 1.0 / 16.0;
    
    return float4(color, 1.0);
}