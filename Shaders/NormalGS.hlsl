#include "Common.hlsli"

struct GSInput
{
    float4 posModel : POSITION;
    float4 normalModel : NORMAL;
};

struct PixelShaderInput
{
    float4 pos : SV_Position;
    float3 color : COLOR;
};

[maxvertexcount(2)]
void main(point GSInput input[1], inout LineStream<PixelShaderInput> outputStream)
{
    PixelShaderInput output;
    
    // Ω√¿€¡°
    float4 posWorld = mul(input[0].posModel, world);
    float4 normalWorld = mul(input[0].normalModel, worldIT);
    normalWorld = float4(normalize(normalWorld.xyz), 0.0);
    
    output.pos = mul(posWorld, view);
    output.pos = mul(output.pos, proj);
    output.color = float3(1.0, 1.0, 0.0);
    outputStream.Append(output);
    
    output.pos = mul(posWorld + normalWorld * 0.1, view);
    output.pos = mul(output.pos, proj);
    output.color = float3(1.0, 0.0, 0.0);
    outputStream.Append(output);
}