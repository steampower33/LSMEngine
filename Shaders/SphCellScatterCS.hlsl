#include "SphCommon.hlsli"

StructuredBuffer<uint> CellStart : register(t9);
StructuredBuffer<uint2> CellOffset : register(t8);

RWStructuredBuffer<uint> SortedIdx : register(u11);

[numthreads(GROUP_SIZE_X, 1, 1)]
void main(uint tid       : SV_GroupThreadID,
    uint3 gtid : SV_DispatchThreadID,
    uint groupIdx : SV_GroupID)
{
    uint i = groupIdx.x * GROUP_SIZE_X + tid;

    if (i >= numParticles) return;

    uint2 info = CellOffset[i];

    if (info.x == 0xFFFFFFFF || info.y == 0xFFFFFFFF)
        return;

    uint dst = CellStart[info.x] + info.y;

    SortedIdx[dst] = i;
}