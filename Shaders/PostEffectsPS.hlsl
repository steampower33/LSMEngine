#include "Common.hlsli" 

Texture2D texture[] : register(t0, space0);

float4 TexcoordToView(float2 texcoord)
{
    float4 posProj;

    // [0, 1]x[0, 1] -> [-1, 1]x[-1, 1]
    posProj.xy = texcoord * 2.0 - 1.0;
    posProj.y *= -1; // 주의: y 방향을 뒤집어줘야 합니다.
    posProj.z = texture[depthOnlySRVIndex].Sample(linearClampSampler, texcoord).r;
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
    if (mode == 1)
    {
        float4 posView = TexcoordToView(input.texcoord);
        float dist = length(posView.xyz); // 눈의 위치가 원점인 좌표계
        //float dist = posView.z;
        
        float3 fogColor = float3(1, 1, 1);
        float fogMin = 4.0;
        float fogMax = 10.0;
        
        float distFog = saturate((dist - fogMin) / (fogMax - fogMin));
        float fogFactor = exp(-distFog * fogStrength);

        // TODO: Fog
        float3 color = texture[resolvedSRVIndex].Sample(linearClampSampler, input.texcoord).rgb;
        
        color = lerp(fogColor, color, fogFactor);
        
        return float4(color, 1);
    }
    else
    {
        float z = TexcoordToView(input.texcoord).z * depthScale;
        return float4(z, z, z, 1);
    }
}
