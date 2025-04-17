#include "SphCommon.hlsli"

RWStructuredBuffer<CompactCell> CompactCells : register(u0);

[numthreads(GROUP_SIZE_X, 1, 1)]
void main(uint tid : SV_GroupThreadID,
	uint3 gtid : SV_DispatchThreadID,
	uint groupIdx : SV_GroupID)
{
	uint globalIndex = groupIdx.x * GROUP_SIZE_X + tid;
	
	CompactCells[globalIndex].cellIndex = 0xFFFFFFFF;
	CompactCells[globalIndex].startIndex = 0xFFFFFFFF;
	CompactCells[globalIndex].endIndex = 0xFFFFFFFF;
}