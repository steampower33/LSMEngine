Texture2D<float4> input : register(t0, space0);
RWTexture2D<float4> output : register(u0, space0);

SamplerState pointClampSampler : register(s0, space0);
SamplerState linearClampSampler : register(s1, space0);

[numthreads(32, 32, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    float width, height;
    output.GetDimensions(width, height);
    
    float2 uv = (DTid.xy + 0.5) / float2(width, height);
    float4 color = input.SampleLevel(linearClampSampler, uv, 0);

    output[DTid.xy] = color;

}