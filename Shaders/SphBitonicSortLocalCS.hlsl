
#include "SphCommon.hlsli" // ParticleHash 정의 포함 가정

RWStructuredBuffer<ParticleHash> hashes : register(u0);

groupshared ParticleHash sharedData[GROUP_SIZE_X];

void SharedMemoryCompareAndSwap(uint index1, uint index2, bool sortAscending)
{
    if (index1 >= GROUP_SIZE_X || index2 >= GROUP_SIZE_X) return;

    ParticleHash hash1 = sharedData[index1];
    ParticleHash hash2 = sharedData[index2];

    // sortAscending == true: 오름차순 정렬
    // sortAscending == false: 내림차순 정렬
    bool shouldSwap = (hash1.cellIndex > hash2.cellIndex);
    if (!sortAscending)
    {
        shouldSwap = !shouldSwap; // 내림차순이면 조건 반전 (작은게 뒤로)
    }

    if (shouldSwap)
    {
        sharedData[index1] = hash2;
        sharedData[index2] = hash1;
    }
}

[numthreads(GROUP_SIZE_X, 1, 1)]
void main(uint3 gtid : SV_DispatchThreadID,
    uint3 gidx : SV_GroupID,
    uint3 dtid : SV_GroupThreadID)
{
    uint globalIndex = gtid.x; // 현재 스레드가 처리할 글로벌 인덱스
    uint localIndex = dtid.x;  // 현재 스레드의 그룹 내 로컬 인덱스

    if (globalIndex < numParticles)
    {
        sharedData[localIndex] = hashes[globalIndex];
    }
    else
    {
        sharedData[localIndex].cellIndex = 0x7FFFFFFF; // int max
        sharedData[localIndex].particleID = 0xFFFFFFFF;
        sharedData[localIndex].flag = 0; // 필요시 초기화
    }

    GroupMemoryBarrierWithGroupSync();

    int particleCnt = (numParticles < GROUP_SIZE_X) ? numParticles : GROUP_SIZE_X;

    // k: 병합할 시퀀스의 크기 (2, 4, 8, ..., GROUP_SIZE_X)
    for (uint k = 2; k <= GROUP_SIZE_X; k *= 2) { // 루프 상한은 GROUP_SIZE_X 유지
        bool sortAscending = ((localIndex & k) == 0);
        for (uint j = k / 2; j > 0; j /= 2) {
            uint partnerIndex = localIndex ^ j;
            if (partnerIndex > localIndex) {
                SharedMemoryCompareAndSwap(localIndex, partnerIndex, sortAscending);
            }
            GroupMemoryBarrierWithGroupSync(); // 각 j 단계 후 동기화
        }
    }

    if (globalIndex < numParticles)
    {
        hashes[globalIndex] = sharedData[localIndex];
    }
}