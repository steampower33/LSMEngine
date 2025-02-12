struct SamplingPSInput
{
    float4 posModel : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

SamplingPSInput main(uint vertexID : SV_VertexID)
{
    SamplingPSInput output;
    
    float2 positions[3] =
    {
        float2(-1.0, 3.0), // v0: 화면 상단 바깥
        float2(-1.0, -1.0), // v1: 왼쪽 아래
        float2(3.0, -1.0) // v2: 오른쪽 아래
    };
    
    float2 uvs[3] =
    {
        float2(0.0, -1.0), // v0
        float2(0.0, 1.0), // v1
        float2(2.0, 1.0) // v2
    };
    
    output.posModel = float4(positions[vertexID], 0.0, 1.0);
    output.texcoord = uvs[vertexID];
    
    return output;
}