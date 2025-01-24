#include "Common.hlsli" 

Texture2D texture[50] : register(t0, space0);
SamplerState wrapSampler : register(s0, space0);

float4 TexcoordToView(float2 texcoord)
{
    float4 posProj;

    // [0, 1]x[0, 1] -> [-1, 1]x[-1, 1]
    posProj.xy = texcoord * 2.0 - 1.0;
    posProj.y *= -1; // 주의: y 방향을 뒤집어줘야 합니다.
    posProj.z = texture[0].Sample(wrapSampler, texcoord).r;
    posProj.w = 1.0;

    // ProjectSpace -> ViewSpace
    float4 posView = mul(posProj, invProj);
    posView.xyz /= posView.w;
    
    return posView;
}

struct SamplingPSInput
{
    float4 posModel : SV_POSITION;
    float2 texcoord : TEXCOORD;
};


float4 main(SamplingPSInput input) : SV_TARGET
{
    float z = TexcoordToView(input.texcoord).z * depthScale;
    return float4(z, z, z, 1);
}
