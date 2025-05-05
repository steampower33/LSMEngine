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
        // 현재 shMem[tid]는 마지막 요소의 exclusive sum
        // 여기에 마지막 요소의 원본 값을 더하면 그룹 전체의 inclusive sum (총 합계)이 됨
        uint totalGroupSum = shMem[tid] + localValue; // localValue는 tid=511 스레드의 원본 값

        // 만약 그룹이 처리하는 마지막 인덱스가 cellCnt를 넘어서는 경우,
        // 해당 그룹의 실제 총 합은 마지막 요소의 exclusive sum + 마지막 요소 값으로 계산된 값과 다를 수 있음.
        // 그러나 일반적으로 PartialSum은 각 그룹이 처리한 범위 내의 '실제 합'을 저장하는 것이 목적.
        // 위 계산(shMem[tid] + localValue)이 대부분의 경우 올바른 그룹 총합임.
        PartialSum[groupIdx] = totalGroupSum;
    }
}
