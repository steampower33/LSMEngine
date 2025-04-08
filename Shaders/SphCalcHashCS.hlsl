#include "SphCommon.hlsli"

RWStructuredBuffer<Particle> Particles : register(u0);
RWStructuredBuffer<ParticleHash> ParticleHashes : register(u1);

[numthreads(GROUP_SIZE_X, 1, 1)]
void main(uint tid : SV_GroupThreadID,
    uint3 gtid : SV_DispatchThreadID,
    uint groupIdx : SV_GroupID)
{
    uint index = groupIdx.x * GROUP_SIZE_X + tid;
    if (index >= maxParticles) return; // 경계 체크

    Particle p = Particles[index];
    ParticleHash result = ParticleHashes[index];

    // 파티클 위치 p.position 을 그리드 최소 바운더리 위치를 기준으로 상대 위치로 변환
    float3 relativePos = p.position - minBounds;

    // 상대 위치를 셀 크기로 나누어, 각 축별로 몇 번째 셀에 해당하는지 실수 값으로 계산
    float3 normalizedPos = relativePos / cellSize;

    // floor 연산으로 소수점을 버려 정수 인덱스를 얻음 (이때 음수 인덱스 가능성 있음)
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