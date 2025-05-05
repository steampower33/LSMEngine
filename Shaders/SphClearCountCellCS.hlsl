#include "SphCommon.hlsli"

RWStructuredBuffer<uint>  CellCount : register(u7);

[numthreads(GROUP_SIZE_X, 1, 1)]
void main(uint tid : SV_GroupThreadID,
    uint3 gtid : SV_DispatchThreadID,
    uint groupIdx : SV_GroupID)
{
    uint i = groupIdx.x * GROUP_SIZE_X + tid;

    CellCount[i] = 0;
}