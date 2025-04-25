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

    float t0 = ParticlesInput[i].spawnTime;
    if (currentTime < t0)
    {
        CellOffset[i] = uint2(0xFFFFFFFF, 0xFFFFFFFF);
        return;
    }

    float3 position = ParticlesInput[i].position;

    // ����� ��ġ�� ��ȯ -> Ŀ�� �ݰ����� ������ -> �ؽ� ��� -> cellIndex
    uint cellIndex = GetCellKeyFromCellID(floor((position - minBounds) / smoothingRadius));

    uint localOff;
    InterlockedAdd(CellCount[cellIndex], 1, localOff);

    CellOffset[i] = uint2(cellIndex, localOff);
}
