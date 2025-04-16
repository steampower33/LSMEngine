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

    // ����� ��ġ�� ��ȯ -> Ŀ�� �ݰ����� ������ -> cellID
    uint cellKey = GetCellKeyFromCellID((p.position - minBounds) / smoothingRadius);

    ParticleHashes[index].particleID = index;
    ParticleHashes[index].cellKey = cellKey;
    ParticleHashes[index].flag = 0;
}