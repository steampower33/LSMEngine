#include "SphCommon.hlsli"

StructuredBuffer<ParticleHash> SortedHashes : register(t0);
RWStructuredBuffer<ScanResult> LocalScan : register(u0);
//RWStructuredBuffer<ScanResult> PartialSum : register(u1);

groupshared uint shMem[GROUP_SIZE_X];

// ���� ��ĵ ����
[numthreads(GROUP_SIZE_X, 1, 1)]
void main(uint tid       : SV_GroupThreadID,
          uint3 gtid     : SV_DispatchThreadID,
          uint groupIdx  : SV_GroupID)
{
    uint globalIndex = groupIdx.x * GROUP_SIZE_X + tid;

    uint loadedFlag = 0;
    if (globalIndex < maxParticles)
    {
        loadedFlag = SortedHashes[globalIndex].flag;
    }

    // ���� �޸𸮿� ����
    shMem[tid] = loadedFlag;
    GroupMemoryBarrierWithGroupSync();

    // (3) Up-Sweep �ܰ�
    for (uint dUp = 1; dUp < GROUP_SIZE_X; dUp <<= 1)
    {
        uint idx = (tid + 1) * (dUp << 1) - 1;
        if (idx < GROUP_SIZE_X)
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
        shMem[GROUP_SIZE_X - 1] = 0;
    }
    GroupMemoryBarrierWithGroupSync();
    
    for (uint dDown = GROUP_SIZE_X >> 1; dDown > 0; dDown >>= 1)
    {
        uint idx = (tid + 1) * (dDown << 1) - 1;
        if (idx < GROUP_SIZE_X)
        {
            uint prev = idx - dDown;
            uint temp = shMem[prev];
            shMem[prev] = shMem[idx];
            shMem[idx] += temp;
        }
        GroupMemoryBarrierWithGroupSync();
    }

    if (globalIndex < maxParticles)
    {
        LocalScan[globalIndex].groupID = shMem[tid];
    }
    GroupMemoryBarrierWithGroupSync();
    
    //if (globalIndex % GROUP_SIZE_X == GROUP_SIZE_X - 1)
    //    PartialSum[globalIndex / GROUP_SIZE_X].groupID = shMem[tid];
}

