#include "SphCommon.hlsli"

StructuredBuffer<ParticleHash> SortedParticleHashes : register(t0);
StructuredBuffer<ScanResult> ScanResults : register(t1);
RWStructuredBuffer<CompactCell> CompactCells : register(u0);

[numthreads(GROUP_SIZE_X, 1, 1)]
void main(uint tid       : SV_GroupThreadID,
	uint3 gtid : SV_DispatchThreadID,
	uint groupIdx : SV_GroupID)
{
	uint globalIndex = groupIdx.x * GROUP_SIZE_X + tid;

	uint maxGroupCnt = ScanResults[MAX_PARTICLES - 1].groupID + 1;

	if (SortedParticleHashes[globalIndex].flag == 1)
	{
		CompactCells[globalIndex].hashValue = SortedParticleHashes[globalIndex].hashValue;
		CompactCells[globalIndex].startIndex = globalIndex;
	}
	GroupMemoryBarrierWithGroupSync();

	CompactCells[globalIndex].endIndex = (globalIndex == maxGroupCnt) ? MAX_PARTICLES : CompactCells[globalIndex + 1].startIndex;
}