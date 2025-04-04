
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
    float3 position : POSITION; // VB의 로컬 좌표 (-1 ~ +1)
    // UV, Normal 등 다른 데이터는 라인 렌더링에 필요 없음
};

struct PSInput
{
    float4 position : SV_Position;
};

PSInput main(VSInput input)
{
    PSInput output;

    // 1. 로컬 좌표 -> 월드 좌표 변환
    float4 worldPos = mul(float4(input.position, 1.0f), world);

    // 2. 월드 좌표 -> 클립 공간 변환
    output.position = mul(worldPos, viewProj);

    return output;
}