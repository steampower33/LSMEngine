#include "SphCommon.hlsli"

cbuffer SimParams : register(b0) {
    float deltaTime;
    float2 gravity;
    uint numParticles;
    float3 minBounds;
    float gridDimX;
    float3 maxBounds;
    float gridDimY;

    float gridDimZ;
    float cellSize;
    float p2;
    float p3;
};

// �Է� ��ƼŬ ���� (SRV)
StructuredBuffer<Particle> ParticlesInput : register(t0);
// ��� (particleID, hashValue) ���� (UAV)
RWStructuredBuffer<ParticleHash> ParticleHashesOutput : register(u0);

[numthreads(GROUP_SIZE_X, 1, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint index = dispatchThreadID.x;
    if (index >= MAX_PARTICLES) return; // ��� üũ

    Particle p = ParticlesInput[index];
    ParticleHash result = ParticleHashesOutput[index];

    // ��ƼŬ ��ġ p.position �� �׸��� �ּ� �ٿ���� ��ġ�� �������� ��� ��ġ�� ��ȯ
    float3 relativePos = p.position - minBounds;

    // ��� ��ġ�� �� ũ��� ������, �� �ະ�� �� ��° ���� �ش��ϴ��� �Ǽ� ������ ���
    float3 normalizedPos = relativePos / cellSize;

    // floor �������� �Ҽ����� ���� ���� �ε����� ���� (�̶� ���� �ε��� ���ɼ� ����)
    int3 cellID = int3(floor(normalizedPos));

    uint hashValue = uint(cellID.x) +
        uint(cellID.y) * gridDimX +
        uint(cellID.z) * gridDimX * gridDimY;

    ParticleHashesOutput[index].particleID = index;
    ParticleHashesOutput[index].hashValue = hashValue;
    ParticleHashesOutput[index].flag = 0;
}