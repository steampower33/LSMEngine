#include "SphCommon.hlsli"

RWStructuredBuffer<Particle> Particles : register(u0);
RWStructuredBuffer<ParticleHash> ParticleHashes : register(u1);

[numthreads(GROUP_SIZE_X, 1, 1)]
void main(uint tid : SV_GroupThreadID,
    uint3 gtid : SV_DispatchThreadID,
    uint groupIdx : SV_GroupID)
{
    uint index = groupIdx.x * GROUP_SIZE_X + tid;
    if (index >= maxParticles) return; // ��� üũ

    Particle p = Particles[index];
    ParticleHash result = ParticleHashes[index];

    // ��ƼŬ ��ġ p.position �� �׸��� �ּ� �ٿ���� ��ġ�� �������� ��� ��ġ�� ��ȯ
    float3 relativePos = p.position - minBounds;

    // ��� ��ġ�� �� ũ��� ������, �� �ະ�� �� ��° ���� �ش��ϴ��� �Ǽ� ������ ���
    float3 normalizedPos = relativePos / cellSize;

    // floor �������� �Ҽ����� ���� ���� �ε����� ���� (�̶� ���� �ε��� ���ɼ� ����)
    int3 cellID = int3(floor(normalizedPos));

    uint hashValue = uint(cellID.x) +
        uint(cellID.y) * gridDimX +
        uint(cellID.z) * gridDimX * gridDimY;

    ParticleHashes[index].particleID = index;
    ParticleHashes[index].hashValue = hashValue;
    ParticleHashes[index].flag = 0;

    Particles[index].density = 0.0;
    Particles[index].force = float3(0.0, 0.0, 0.0);
    Particles[index].pressure = 0.0;
}