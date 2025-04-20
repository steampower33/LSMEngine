#include "SphCommon.hlsli"

RWStructuredBuffer<ParticleHash> SortedHashes : register(u0);

[numthreads(GROUP_SIZE_X, 1, 1)]
void main(uint tid : SV_GroupThreadID,
    uint3 gtid : SV_DispatchThreadID,
    uint groupIdx : SV_GroupID)
{
    uint globalIndex = groupIdx.x * GROUP_SIZE_X + tid;

    if (globalIndex == 0)
        SortedHashes[globalIndex].flag = 1;
    else if (globalIndex >= numParticles)
        return;
    
    if (SortedHashes[globalIndex - 1].cellIndex != SortedHashes[globalIndex].cellIndex)
        SortedHashes[globalIndex].flag = 1;
}