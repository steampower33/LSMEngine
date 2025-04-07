#include "SphCommon.hlsli"

cbuffer BitonicParams : register(b1)
{
    uint k;
    uint j;
}

RWStructuredBuffer<ParticleHash> hashes : register(u0);

[numthreads(GROUP_SIZE_X, 1, 1)]
void main(uint tid : SV_GroupThreadID,
    uint3 gtid : SV_DispatchThreadID,
    uint groupIdx : SV_GroupID)
{
    uint i = groupIdx.x * GROUP_SIZE_X + tid;

    uint l = i ^ j;
    if (l > i)
    {
        ParticleHash iHash = hashes[i];
        ParticleHash lHash = hashes[l];

        if (((i & k) == 0 && iHash.hashValue > lHash.hashValue) ||
            ((i & k) != 0 && iHash.hashValue < lHash.hashValue))
        {
            hashes[i] = lHash;
            hashes[l] = iHash;
        }
    }
}