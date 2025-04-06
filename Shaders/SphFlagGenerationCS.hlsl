#include "SphCommon.hlsli"

RWStructuredBuffer<ParticleHash> SortedHashes : register(u0);

[numthreads(GROUP_SIZE_X, 1, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint index = dispatchThreadID.x;
    if (index == 0)
        SortedHashes[index].flag = 1;
    else if (index >= MAX_PARTICLES)
        return;
    
    if (SortedHashes[index - 1].hashValue != SortedHashes[index].hashValue)
        SortedHashes[index].flag = 1;
}