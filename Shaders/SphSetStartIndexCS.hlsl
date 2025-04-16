#include "SphCommon.hlsli"

StructuredBuffer<ParticleHash> SortedHashes : register(t0);
RWStructuredBuffer<CellStart> CellStarts : register(u0);

[numthreads(GROUP_SIZE_X, 1, 1)]
void main(uint tid : SV_GroupThreadID,
    uint3 gtid : SV_DispatchThreadID,
    uint groupIdx : SV_GroupID)
{
    uint globalIndex = groupIdx.x * GROUP_SIZE_X + tid;

    if (globalIndex == 0)
        CellStarts[SortedHashes[globalIndex].cellKey].startIndex = globalIndex;

    if (SortedHashes[globalIndex - 1].cellKey != SortedHashes[globalIndex].cellKey)
        CellStarts[SortedHashes[globalIndex].cellKey].startIndex = globalIndex;
}