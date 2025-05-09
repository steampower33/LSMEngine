
cbuffer GlobalConstants : register(b0)
{
    float4x4 view;
    float4x4 proj;
    float4x4 viewProj;
    float4x4 invProj;

    float3 eyeWorld;
    float strengthIBL;

    int choiceEnvMap;
    float envLodBias;
    int mode;
    float depthScale;

    float fogStrength;
    uint depthOnlySRVIndex;
    uint shadowDepthOnlyStartIndex;
    uint resolvedSRVIndex;

    uint fogSRVIndex;
    uint isEnvEnabled;
    float d01;
    float d02;

    float4x4 d03;
    float4x4 d04;
    float4x4 d05;
}

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
    float p3;
};

struct GSInput
{
    float4 viewPos : SV_POSITION;
    float radius : PSIZE;
};

StructuredBuffer<float3> Positions : register(t0);

GSInput main(uint vertexID : SV_VertexID)
{
    const float fadeLife = 0.2f;

    float3 pos = Positions[vertexID];

    GSInput output;

    output.viewPos = mul(float4(pos, 1.0), view);
    output.radius = radius;

    return output;
}
