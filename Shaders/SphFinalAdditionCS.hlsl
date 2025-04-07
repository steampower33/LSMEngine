#include "SphCommon.hlsli"

StructuredBuffer<ScanResult> BlockSum : register(t0);
RWStructuredBuffer<ScanResult> LocalScan : register(u0);

[numthreads(GROUP_SIZE_X, 1, 1)]
void main(uint tid       : SV_GroupThreadID,
    uint3 gtid : SV_DispatchThreadID,
    uint groupIdx : SV_GroupID)
{
    uint globalIndex = groupIdx.x * GROUP_SIZE_X + tid;

    if (globalIndex < GROUP_SIZE_X)
        return;

    LocalScan[globalIndex].groupID += BlockSum[globalIndex / GROUP_SIZE_X].groupID;
}