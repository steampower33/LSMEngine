#include "SphCommon.hlsli"

RWStructuredBuffer<uint> PartialSum : register(u0);

groupshared uint shMem[GROUP_SIZE_X];

[numthreads(GROUP_SIZE_X, 1, 1)]
void main(uint tid       : SV_GroupThreadID,
    uint3 gtid : SV_DispatchThreadID,
    uint groupIdx : SV_GroupID)
{
    uint i = groupIdx.x * GROUP_SIZE_X + tid;

    if (i >= (cellCnt + GROUP_SIZE_X - 1) / GROUP_SIZE_X) return;

    shMem[tid] = PartialSum[i];

    GroupMemoryBarrierWithGroupSync();

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

    PartialSum[i] = shMem[tid];

    GroupMemoryBarrierWithGroupSync();
}
