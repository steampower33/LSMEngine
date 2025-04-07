#include "SphCommon.hlsli"

StructuredBuffer<ParticleHash> SortedParticleHashes : register(t0);
StructuredBuffer<CompactCell> CompactCells : register(t1);
StructuredBuffer<int> CellMap : register(t2);

RWStructuredBuffer<Particle> Particles : register(u0);

cbuffer SimParams : register(b0) {
    float deltaTime;
    float2 gravity;
    uint numParticles;
    float3 minBounds;
    float gridDimX;
    float3 maxBounds;
    float gridDimY;

    float gridDimZ;
    float cellSize;
    float p2;
    float p3;
};


[numthreads(GROUP_SIZE_X, 1, 1)]
void main(uint tid : SV_GroupThreadID,
    uint3 gtid : SV_DispatchThreadID,
    uint groupIdx : SV_GroupID)
{
    uint index = groupIdx.x * GROUP_SIZE_X + tid;

	Particle p = Particles[index];

    float3 relativePos = p.position - minBounds;
    float3 normalizedPos = relativePos / cellSize;
    int3 cellID = int3(floor(normalizedPos));

    for (int offsetY = -1; offsetY <= 1; ++offsetY)
    {
        for (int offsetX = -1; offsetX <= 1; ++offsetX)
        {
            int nearCellX = cellID[0] + offsetX;
            int nearCellY = cellID[1] + offsetY;

            int groupId = CellMap[nearCellX + nearCellY * gridDimX];
            CompactCell compact = CompactCells[groupId];

        }
    }

    Particles[index].density = 0.0;
}