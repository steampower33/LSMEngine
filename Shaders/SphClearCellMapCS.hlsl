#include "SphCommon.hlsli"

RWStructuredBuffer<int> CellMap : register(u0);

[numthreads(GROUP_SIZE_X, 1, 1)]
void main(uint tid : SV_GroupThreadID,
	uint3 gtid : SV_DispatchThreadID,
	uint groupIdx : SV_GroupID)
{
	uint globalIndex = groupIdx.x * GROUP_SIZE_X + tid;
	if (globalIndex >= cellCnt) return;
	CellMap[globalIndex] = -1;
}