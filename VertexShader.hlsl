
cbuffer SceneConstantBuffer : register(b0)
{
    float4 offset;
    float4x4 world;
    float4x4 view;
    float4x4 proj;
    float padding[12];
}

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
    
    float4 worldPosition = mul(input.position, world);
    float4 viewPosition = mul(worldPosition, view);
    result.position = mul(viewPosition, proj);
    
    result.position += offset;
    
    result.texcoord = input.texcoord;

    return result;
}