#include "Common.hlsli"

Texture2D g_texture[] : register(t0, space1);
SamplerState g_sampler : register(s0, space0);

struct SamplingPSInput
{
    float4 posModel : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

float3 LinearToneMapping(float3 color)
{
    float3 invGamma = float3(1, 1, 1) / gamma;

    color = clamp(exposure * color, 0., 1.);
    color = pow(color, invGamma);
    return color;
}

float4 main(SamplingPSInput input) : SV_TARGET
{
    float3 color0 = g_texture[0].Sample(g_sampler, input.texcoord).xyz;
    float3 color1 = g_texture[1].Sample(g_sampler, input.texcoord).xyz;
    
    float3 combined = (1.0 - strength) * color0 + strength * color1;
    
    combined = LinearToneMapping(combined);
    
    return float4(combined, 1.0f);
}
