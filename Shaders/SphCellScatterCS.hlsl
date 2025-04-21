#include "SphCommon.hlsli"

StructuredBuffer<uint> CellStart : register(t0);
StructuredBuffer<uint2> CellOffset : register(t1);

RWStructuredBuffer<uint> SortedIdx : register(u0);

[numthreads(GROUP_SIZE_X, 1, 1)]
void main(uint tid       : SV_GroupThreadID,
    uint3 gtid : SV_DispatchThreadID,
    uint groupIdx : SV_GroupID)
{
    uint i = groupIdx.x * GROUP_SIZE_X + tid;

    if (i >= numParticles) return;

    uint2 info = CellOffset[i];

    uint dst = CellStart[info.x] + info.y;

    SortedIdx[dst] = i;
}