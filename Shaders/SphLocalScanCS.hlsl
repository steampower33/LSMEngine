#include "SphCommon.hlsli"

StructuredBuffer<ParticleHash> SortedHashes : register(t0);
RWStructuredBuffer<ScanResult> LocalScan : register(u0);
RWStructuredBuffer<ScanResult> PartialSum : register(u1);

groupshared uint shMem[GROUP_SIZE_X];

#define TEST_SIZE 4

// ���� ��ĵ ����
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

    // ���� �޸𸮿� ����
    shMem[tid] = loadedFlag;
    GroupMemoryBarrierWithGroupSync();

    // (3) Up-Sweep �ܰ�
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
    // ������ ����(= ��ü ��)�� 0�� ����->Blelloch ǥ�� �˰���
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

