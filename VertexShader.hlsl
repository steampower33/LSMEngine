
//cbuffer SceneConstantBuffer : register(b0)
//{
//    float4 offset;
//    float4 padding[15];
//}

struct VertexShaderInput
{
    float4 position : POSITION;
    float2 texcoord : TEXCOORD;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

PSInput main(VertexShaderInput input)
{
    PSInput result;

    result.position = input.position;
    result.texcoord = input.texcoord;

    return result;
}