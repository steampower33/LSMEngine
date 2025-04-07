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

// 입력 파티클 버퍼 (SRV)
StructuredBuffer<Particle> ParticlesInput : register(t0);
// 출력 (particleID, hashValue) 버퍼 (UAV)
RWStructuredBuffer<ParticleHash> ParticleHashesOutput : register(u0);

[numthreads(GROUP_SIZE_X, 1, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint index = dispatchThreadID.x;
    if (index >= MAX_PARTICLES) return; // 경계 체크

    Particle p = ParticlesInput[index];
    ParticleHash result = ParticleHashesOutput[index];

    // 파티클 위치 p.position 을 그리드 최소 바운더리 위치를 기준으로 상대 위치로 변환
    float3 relativePos = p.position - minBounds;

    // 상대 위치를 셀 크기로 나누어, 각 축별로 몇 번째 셀에 해당하는지 실수 값으로 계산
    float3 normalizedPos = relativePos / cellSize;

    // floor 연산으로 소수점을 버려 정수 인덱스를 얻음 (이때 음수 인덱스 가능성 있음)
    int3 cellID = int3(floor(normalizedPos));

    uint hashValue = uint(cellID.x) +
        uint(cellID.y) * gridDimX +
        uint(cellID.z) * gridDimX * gridDimY;

    ParticleHashesOutput[index].particleID = index;
    ParticleHashesOutput[index].hashValue = hashValue;
    ParticleHashesOutput[index].flag = 0;
}