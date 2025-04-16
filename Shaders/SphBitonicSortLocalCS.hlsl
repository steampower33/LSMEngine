
#include "SphCommon.hlsli" // ParticleHash 정의 포함 가정

RWStructuredBuffer<ParticleHash> DataBuffer : register(u0);

cbuffer SortParams : register(b0)
{
    uint numElements;
};

groupshared ParticleHash sharedData[GROUP_SIZE_X];

void SharedMemoryCompareAndSwap(uint index1, uint index2, bool sortAscending)
{
    if (index1 >= GROUP_SIZE_X || index2 >= GROUP_SIZE_X) return;

    ParticleHash hash1 = sharedData[index1];
    ParticleHash hash2 = sharedData[index2];

    // sortAscending == true: 오름차순 정렬
    // sortAscending == false: 내림차순 정렬
    bool shouldSwap = (hash1.cellKey > hash2.cellKey);
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

    if (globalIndex < numElements)
    {
        sharedData[localIndex] = DataBuffer[globalIndex];
    }
    else
    {
        // 여기서는 간단히 cellKey를 매우 큰 값으로 설정 (오름차순 기준)
        sharedData[localIndex].cellKey = 0xFFFFFFFF; // 또는 적절한 최대값
        sharedData[localIndex].particleID = 0xFFFFFFFF;
    }

    GroupMemoryBarrierWithGroupSync();

    // k: 병합할 시퀀스의 크기 (2, 4, 8, ..., GROUP_SIZE_X)
    for (uint k = 2; k <= GROUP_SIZE_X; k *= 2)
    {
        // 현재 스레드가 결정해야 할 정렬 방향 (Bitonic Merge 단계)
        // (i & k) == 0 부분과 유사한 역할 -> 스레드 인덱스를 k 크기의 블록으로 나누어 방향 결정
        bool sortAscending = ((localIndex & k) == 0);

        // j: 비교할 요소 간의 거리 (k/2, k/4, ..., 1)
        for (uint j = k / 2; j > 0; j /= 2)
        {
            // 비교할 상대방 인덱스 계산
            uint partnerIndex = localIndex ^ j; // XOR 연산으로 파트너 찾기

            // 파트너가 자기 자신보다 뒤에 있는 경우에만 비교 수행 (중복 방지)
            // 또한 파트너가 같은 k-블록 내에 있는지 확인 (상위 비트가 같아야 함) -> 이 부분은 j < k/2 에서 자연스럽게 만족
            if (partnerIndex > localIndex)
            {
                SharedMemoryCompareAndSwap(localIndex, partnerIndex, sortAscending);
            }

            GroupMemoryBarrierWithGroupSync();
        }
    }

    if (globalIndex < numElements)
    {
        DataBuffer[globalIndex] = sharedData[localIndex];
    }
}