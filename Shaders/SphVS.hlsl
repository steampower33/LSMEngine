
cbuffer RenderParams : register(b0)
{
    float4x4 view;
    float4x4 proj;

    float thicknessContributionScale;
    float p1;
    float p2;
    float p3;

    float4 p4;
    float4 p5;
    float4 p6;

    float4x4 p7;
};

cbuffer SimParams : register(b1) {
    float deltaTime;
    uint numParticles;
    float smoothingRadius;
    uint cellCnt;

    float3 minBounds;
    float currentTime;
    float3 maxBounds;
    float endTime;

    int gridDimX;
    int gridDimY;
    int gridDimZ;
    uint forceKey;

    float density0;
    float pressureCoeff;
    float nearPressureCoeff;
    float viscosity;

    float mass;
    float radius;
    float boundaryStiffness;
    float boundaryDamping;

    float gravityCoeff;
    float duration;
    float startTime;
    float p8;
};

struct GSInput
{
    float3 viewPos : POSITION;
    float radius : PSIZE0;
};

StructuredBuffer<float3> Positions : register(t0);

GSInput main(uint vertexID : SV_VertexID)
{
    float3 pos = Positions[vertexID];

    GSInput output;

    output.viewPos = mul(float4(pos, 1.0), view).xyz;
    output.radius = radius;

    return output;
}
