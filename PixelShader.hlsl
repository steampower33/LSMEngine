struct PSInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

Texture2D g_texture : register(t0);
SamplerState g_sampler : register(s0);


float4 main(PSInput input) : SV_TARGET
{
    return g_texture.Sample(g_sampler, input.texcoord);
}
