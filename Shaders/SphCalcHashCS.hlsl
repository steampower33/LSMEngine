#include "SphCommon.hlsli"

StructuredBuffer<Particle> Particles : register(t0);
RWStructuredBuffer<ParticleHash> ParticleHashes : register(u0);

[numthreads(GROUP_SIZE_X, 1, 1)]
void main(uint tid : SV_GroupThreadID,
    uint3 gtid : SV_DispatchThreadID,
    uint groupIdx : SV_GroupID)
{
    uint index = groupIdx.x * GROUP_SIZE_X + tid;

    if (index >= maxParticles) return; // ��� üũ

    Particle p = Particles[index];

    // ��ƼŬ ��ġ p.position �� �׸��� �ּ� �ٿ���� ��ġ�� �������� ��� ��ġ�� ��ȯ
    float3 relativePos = p.position - minBounds;

    // ��� ��ġ�� �� ũ��� ������, �� �ະ�� �� ��° ���� �ش��ϴ��� �Ǽ� ������ ���
    float3 normalizedPos = relativePos / 1;

    // floor �������� �Ҽ����� ���� ���� �ε����� ���� (�̶� ���� �ε��� ���ɼ� ����)
    int3 cellID = int3(floor(normalizedPos));

    uint hashValue = uint(cellID.x) +
        uint(cellID.y) * gridDimX +
        uint(cellID.z) * gridDimX * gridDimY;

    ParticleHashes[index].particleID = index;
    ParticleHashes[index].hashValue = hashValue;
    ParticleHashes[index].flag = 0;
}