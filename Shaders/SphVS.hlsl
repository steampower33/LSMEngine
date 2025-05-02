
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

struct Particle {
    float3 position;
    float3 velocity;
    float3 predictedPosition;
    float density;
    float nearDensity;
    float spawnTime;
    float p;
};

struct GSInput
{
    float4 viewPos : SV_POSITION;
    float3 color : COLOR;
    float life : PSIZE0;
    float radius : PSIZE1;
    uint  isGhost : TEXCOORD0;
};

StructuredBuffer<Particle> particles : register(t0);

GSInput main(uint vertexID : SV_VertexID)
{
    const float fadeLife = 0.2f;

    Particle p = particles[vertexID];

    GSInput output;

    output.viewPos = mul(float4(p.position.xyz, 1.0), view);

    output.color = float3(0.0, 0.0, 0.0);
    output.radius = 0.1;

    return output;
}
