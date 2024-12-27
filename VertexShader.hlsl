
cbuffer SceneConstantBuffer : register(b0)
{
    float4x4 world;
    float4x4 view;
    float4x4 proj;
    float offset;
    float padding[15];
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
    
    result.position.x += offset;
    
    result.texcoord = input.texcoord;

    return result;
}