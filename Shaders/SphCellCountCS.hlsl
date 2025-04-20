#include "SphCommon.hlsli"

StructuredBuffer<Particle> ParticlesInput : register(t0);
RWStructuredBuffer<uint>  CellCount : register(u1);
RWStructuredBuffer<uint2> CellOffset : register(u2);

[numthreads(GROUP_SIZE_X, 1, 1)]
void main(uint tid : SV_GroupThreadID,
    uint3 gtid : SV_DispatchThreadID,
    uint groupIdx : SV_GroupID)
{
    uint i = groupIdx.x * GROUP_SIZE_X + tid;
    if (i >= numParticles) return;

    float3 position = ParticlesInput[i].position;

    // ����� ��ġ�� ��ȯ -> Ŀ�� �ݰ����� ������ -> �ؽ� ��� -> cellIndex
    float3 fx = (position - minBounds) / smoothingRadius;
    fx = max(fx, 0.0);
    int3 cellID = int3(floor(fx));
    cellID = clamp(cellID, int3(0, 0, 0), int3(gridDimX, gridDimY, gridDimZ) - int3(1, 1, 1));

    uint cellIndex = GetCellKeyFromCellID(cellID);

    uint localOff;
    InterlockedAdd(CellCount[cellIndex], 1, localOff);

    CellOffset[i] = uint2(cellIndex, localOff);
}
