#include "SphCommon.hlsli"

StructuredBuffer<Particle> ParticlesInput : register(t0);
RWStructuredBuffer<uint>  CellCount : register(u0);
RWStructuredBuffer<uint2> CellOffset : register(u1);

[numthreads(GROUP_SIZE_X, 1, 1)]
void main(uint tid : SV_GroupThreadID,
    uint3 gtid : SV_DispatchThreadID,
    uint groupIdx : SV_GroupID)
{
    uint i = groupIdx.x * GROUP_SIZE_X + tid;
    if (i >= numParticles) return;

    if (currentTime < ParticlesInput[i].spawnTime)
    {
        CellOffset[i] = uint2(0xFFFFFFFF, 0xFFFFFFFF);
        return;
    }

    uint cellIndex = GetCellKeyFromCellID(floor((ParticlesInput[i].predictedPosition - minBounds) / smoothingRadius));

    uint localOff;
    InterlockedAdd(CellCount[cellIndex], 1, localOff);

    CellOffset[i] = uint2(cellIndex, localOff);
}
