#include "SphCommon.hlsli"

StructuredBuffer<float3> predictedPositions : register(t1);
StructuredBuffer<float> spawnTimes : register(t6);

RWStructuredBuffer<uint>  CellCount : register(u7);
RWStructuredBuffer<uint2> CellOffset : register(u8);

[numthreads(GROUP_SIZE_X, 1, 1)]
void main(uint tid : SV_GroupThreadID,
    uint3 gtid : SV_DispatchThreadID,
    uint groupIdx : SV_GroupID)
{
    uint index = groupIdx.x * GROUP_SIZE_X + tid;
    if (index >= numParticles) return;

    if (currentTime < spawnTimes[index])
    {
        CellOffset[index] = uint2(0xFFFFFFFF, 0xFFFFFFFF);
        return;
    }

    uint cellIndex = GetCellKeyFromCellID(floor((predictedPositions[index] - minBounds) / smoothingRadius));

    uint localOff;
    InterlockedAdd(CellCount[cellIndex], 1, localOff);

    CellOffset[index] = uint2(cellIndex, localOff);
}
