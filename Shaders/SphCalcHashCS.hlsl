#include "SphCommon.hlsli"

StructuredBuffer<Particle> Particles : register(t0);
RWStructuredBuffer<ParticleHash> ParticleHashes : register(u0);

[numthreads(GROUP_SIZE_X, 1, 1)]
void main(uint tid : SV_GroupThreadID,
    uint3 gtid : SV_DispatchThreadID,
    uint groupIdx : SV_GroupID)
{
    uint index = groupIdx.x * GROUP_SIZE_X + tid;

    if (index >= numParticles) return; // 경계 체크

    Particle p = Particles[index];

    // 상대적 위치로 변환 -> 커널 반경으로 나눠줌 -> cellID
    uint cellIndex = GetCellKeyFromCellID(floor((p.position - minBounds) / smoothingRadius));

    ParticleHashes[index].particleID = index;
    ParticleHashes[index].cellIndex = cellIndex;
    ParticleHashes[index].flag = 0;
}