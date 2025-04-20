#include "SphCommon.hlsli"

RWStructuredBuffer<uint> LocalScan : register(u0);
RWStructuredBuffer<uint> PartialSum : register(u1);

[numthreads(GROUP_SIZE_X, 1, 1)]
void main(uint tid       : SV_GroupThreadID,
    uint3 gtid : SV_DispatchThreadID,
    uint groupIdx : SV_GroupID)
{
    uint i = groupIdx.x * GROUP_SIZE_X + tid;

    if (i < GROUP_SIZE_X)
        return;

    LocalScan[i] += PartialSum[i / GROUP_SIZE_X];
}