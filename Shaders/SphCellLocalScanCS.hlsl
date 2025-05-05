#include "SphCommon.hlsli"

StructuredBuffer<uint> CellCount : register(t7);
RWStructuredBuffer<uint> LocalScan : register(u9);
RWStructuredBuffer<uint> PartialSum : register(u10);

groupshared uint shMem[GROUP_SIZE_X];

[numthreads(GROUP_SIZE_X, 1, 1)]
void main(uint tid       : SV_GroupThreadID,
    uint3 gtid : SV_DispatchThreadID,
    uint groupIdx : SV_GroupID)
{
    uint i = groupIdx.x * GROUP_SIZE_X + tid;

    uint last_element_original_value = 0;
    uint last_element_global_index = groupIdx.x * GROUP_SIZE_X + (GROUP_SIZE_X - 1);
    if (last_element_global_index < cellCnt) {
        last_element_original_value = CellCount[last_element_global_index];
    }

    uint localValue = 0;
    if (i < cellCnt) {
        localValue = CellCount[i];
    }

    shMem[tid] = localValue;

    GroupMemoryBarrierWithGroupSync();

    for (uint dUp = 1; dUp < GROUP_SIZE_X; dUp <<= 1)
    {
        uint idx = (tid + 1) * (dUp << 1) - 1;
        if (idx < GROUP_SIZE_X)
        {
            uint prev = idx - dUp;
            shMem[idx] += shMem[prev];
        }
        GroupMemoryBarrierWithGroupSync();
    }

    if (tid == 0)
    {
        shMem[GROUP_SIZE_X - 1] = 0;
    }
    GroupMemoryBarrierWithGroupSync();

    for (uint dDown = GROUP_SIZE_X >> 1; dDown > 0; dDown >>= 1)
    {
        uint idx = (tid + 1) * (dDown << 1) - 1;
        if (idx < GROUP_SIZE_X)
        {
            uint prev = idx - dDown;
            uint temp = shMem[prev];
            shMem[prev] = shMem[idx];
            shMem[idx] += temp;
        }
        GroupMemoryBarrierWithGroupSync();
    }

    if (i < cellCnt) {
        LocalScan[i] = shMem[tid];
    }

    GroupMemoryBarrierWithGroupSync();

    if (tid == GROUP_SIZE_X - 1) {
        // ���� shMem[tid]�� ������ ����� exclusive sum
        // ���⿡ ������ ����� ���� ���� ���ϸ� �׷� ��ü�� inclusive sum (�� �հ�)�� ��
        uint totalGroupSum = shMem[tid] + localValue; // localValue�� tid=511 �������� ���� ��

        // ���� �׷��� ó���ϴ� ������ �ε����� cellCnt�� �Ѿ�� ���,
        // �ش� �׷��� ���� �� ���� ������ ����� exclusive sum + ������ ��� ������ ���� ���� �ٸ� �� ����.
        // �׷��� �Ϲ������� PartialSum�� �� �׷��� ó���� ���� ���� '���� ��'�� �����ϴ� ���� ����.
        // �� ���(shMem[tid] + localValue)�� ��κ��� ��� �ùٸ� �׷� ������.
        PartialSum[groupIdx] = totalGroupSum;
    }
}
