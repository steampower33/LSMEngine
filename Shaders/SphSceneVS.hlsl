struct SamplingPSInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

SamplingPSInput main(uint vertexID : SV_VertexID)
{
    SamplingPSInput output;

    output.texcoord = float2((vertexID << 1) & 2, vertexID & 2); // ������ ��Ʈ ����!
    output.position = float4(output.texcoord * 2.0f - 1.0f, 0.0f, 1.0f);
    output.position.y = -output.position.y; // Y�� ���� (DirectX Clip space)

    return output;
}