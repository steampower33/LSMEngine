#include "SphCommon.hlsli"

cbuffer BitonicParams : register(b1)
{
    uint k;
    uint j;
}

RWStructuredBuffer<ParticleHash> hashes : register(u0);

[numthreads(GROUP_SIZE_X, 1, 1)]
void main(uint3 gtid : SV_DispatchThreadID)
{
    uint i = gtid.x;

    uint l = i ^ j; // 파트너 인덱스

    if (l > i)
    {
        ParticleHash iHash = hashes[i];
        ParticleHash lHash = hashes[l];

        bool sortAscending = ((i & k) == 0);

        bool shouldSwap = (iHash.cellKey > lHash.cellKey);
        if (!sortAscending)
        {
            shouldSwap = !shouldSwap;
        }

        if (shouldSwap)
        {
            hashes[i] = lHash;
            hashes[l] = iHash;
        }
    }
}