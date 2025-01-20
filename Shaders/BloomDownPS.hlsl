#include "Common.hlsli"

Texture2D g_texture[] : register(t0, space1);
SamplerState g_sampler : register(s0, space0);

struct SamplingPSInput
{
    float4 posModel : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

float4 main(SamplingPSInput input) : SV_TARGET
{
    float x = input.texcoord.x;
    float y = input.texcoord.y;
    
    float3 a = g_texture[index].Sample(g_sampler, float2(x - 2 * dx, y + 2 * dy)).rgb;
    float3 b = g_texture[index].Sample(g_sampler, float2(x, y + 2 * dy)).rgb;
    float3 c = g_texture[index].Sample(g_sampler, float2(x + 2 * dx, y + 2 * dy)).rgb;
    
    float3 d = g_texture[index].Sample(g_sampler, float2(x - 2 * dx, y)).rgb;
    float3 e = g_texture[index].Sample(g_sampler, float2(x, y)).rgb;
    float3 f = g_texture[index].Sample(g_sampler, float2(x + 2 * dx, y)).rgb;
    
    float3 g = g_texture[index].Sample(g_sampler, float2(x - 2 * dx, y - 2 * dy)).rgb;
    float3 h = g_texture[index].Sample(g_sampler, float2(x, y - 2 * dy)).rgb;
    float3 i = g_texture[index].Sample(g_sampler, float2(x + 2 * dx, y - 2 * dy)).rgb;
    
    float3 j = g_texture[index].Sample(g_sampler, float2(x - dx, y + dy)).rgb;
    float3 k = g_texture[index].Sample(g_sampler, float2(x + dx, y + dy)).rgb;
    float3 l = g_texture[index].Sample(g_sampler, float2(x - dx, y - dy)).rgb;
    float3 m = g_texture[index].Sample(g_sampler, float2(x + dx, y - dy)).rgb;
    
    float3 color = e * 0.125;
    color += (a + c + g + i) * 0.03125;
    color += (b + d + f + h) * 0.0625;
    color += (j + k + l + m) * 0.125;
    
    return float4(color, 1.0);
}