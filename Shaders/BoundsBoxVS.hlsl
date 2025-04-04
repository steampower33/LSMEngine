
cbuffer GlobalConstants : register(b0)
{
    float4x4 view;
    float4x4 proj;
    float4x4 viewProj;
    float4x4 invProj;
};

cbuffer MeshConstants : register(b1)
{
    float4x4 world;

    float4x4 worldIT;
}

struct VSInput
{
    float3 position : POSITION; // VB�� ���� ��ǥ (-1 ~ +1)
    // UV, Normal �� �ٸ� �����ʹ� ���� �������� �ʿ� ����
};

struct PSInput
{
    float4 position : SV_Position;
};

PSInput main(VSInput input)
{
    PSInput output;

    // 1. ���� ��ǥ -> ���� ��ǥ ��ȯ
    float4 worldPos = mul(float4(input.position, 1.0f), world);

    // 2. ���� ��ǥ -> Ŭ�� ���� ��ȯ
    output.position = mul(worldPos, viewProj);

    return output;
}