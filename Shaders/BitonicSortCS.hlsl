struct ParticleHash
{
    uint particleID; // 원래 파티클 인덱스
    uint hashValue;  // 계산된 해시 값
};


cbuffer BitonicParams : register(b1)
{
    uint k;
    uint j;
}

#define GROUP_SIZE_X 256

RWStructuredBuffer<ParticleHash> hashes : register(u0);

[numthreads(GROUP_SIZE_X, 1, 1)]
void main(int3 gID : SV_GroupID, int3 gtID : SV_GroupThreadID,
    uint3 dtID : SV_DispatchThreadID)
{
    uint i = dtID.x;

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