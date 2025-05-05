#include "SphCommon.hlsli"

StructuredBuffer<uint> PartialSum : register(t10);

RWStructuredBuffer<uint> LocalScan : register(u9);

[numthreads(GROUP_SIZE_X, 1, 1)]
void main(uint tid       : SV_GroupThreadID,
    uint3 gtid : SV_DispatchThreadID,
    uint groupIdx : SV_GroupID)
{
    uint i = groupIdx.x * GROUP_SIZE_X + tid;

    LocalScan[i] += PartialSum[i / GROUP_SIZE_X];
}