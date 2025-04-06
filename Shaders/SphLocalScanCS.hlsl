#include "SphCommon.hlsli"

StructuredBuffer<ParticleHash> SortedHashes : register(t0);
RWStructuredBuffer<ScanResult> LocalScan : register(u0);
RWStructuredBuffer<ScanResult> PartialSum : register(u1);

groupshared uint shMem[GROUP_SIZE_X];

#define TEST_SIZE 4

// 병렬 스캔 구현
[numthreads(TEST_SIZE, 1, 1)]
void main(uint tid       : SV_GroupThreadID,
          uint3 gtid     : SV_DispatchThreadID,
          uint groupIdx  : SV_GroupID)
{
    uint globalIndex = groupIdx.x * TEST_SIZE + tid;

    uint loadedFlag = 0;
    if (globalIndex < MAX_PARTICLES)
    {
        loadedFlag = SortedHashes[globalIndex].flag;
    }

    // 공유 메모리에 복사
    shMem[tid] = loadedFlag;
    GroupMemoryBarrierWithGroupSync();

    // (3) Up-Sweep 단계
    for (uint dUp = 1; dUp < TEST_SIZE; dUp <<= 1)
    {
        uint idx = (tid + 1) * (dUp << 1) - 1;
        if (idx < TEST_SIZE)
        {
            uint prev = idx - dUp;
            shMem[idx] += shMem[prev];
        }
        GroupMemoryBarrierWithGroupSync();
    }

    // Down-Sweep
    // 마지막 원소(= 전체 합)에 0을 세팅->Blelloch 표준 알고리즘
    if (tid == 0)
    {
        shMem[TEST_SIZE - 1] = 0;
    }
    GroupMemoryBarrierWithGroupSync();
    
    for (uint dDown = TEST_SIZE >> 1; dDown > 0; dDown >>= 1)
    {
        uint idx = (tid + 1) * (dDown << 1) - 1;
        if (idx < TEST_SIZE)
        {
            uint prev = idx - dDown;
            uint temp = shMem[prev];
            shMem[prev] = shMem[idx];
            shMem[idx] += temp;
        }
        GroupMemoryBarrierWithGroupSync();
    }

    if (globalIndex < MAX_PARTICLES)
    {
        LocalScan[globalIndex].groupID = shMem[tid];
    }
    GroupMemoryBarrierWithGroupSync();
    
    if (globalIndex % 4 == TEST_SIZE - 1)
        PartialSum[globalIndex / 4].groupID = shMem[tid];
}

