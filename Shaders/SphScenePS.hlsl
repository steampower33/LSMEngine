
Texture2D<float4> Shaded : register(t0);

SamplerState SamplerLinearClamp : register(s0);

struct SamplingPSInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

float4 main(SamplingPSInput input) : SV_TARGET
{
    return Shaded.Sample(SamplerLinearClamp, input.texcoord);
}
