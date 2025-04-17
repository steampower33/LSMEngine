#include "SphCommon.hlsli"

StructuredBuffer<ParticleHash> SortedParticleHashes : register(t0);
StructuredBuffer<ScanResult> ScanResults : register(t1);
RWStructuredBuffer<CompactCell> CompactCells : register(u0);
RWStructuredBuffer<int> CellMap : register(u1);

[numthreads(GROUP_SIZE_X, 1, 1)]
void main(uint tid : SV_GroupThreadID,
	uint3 gtid : SV_DispatchThreadID,
	uint groupIdx : SV_GroupID)
{
	uint globalIndex = groupIdx.x * GROUP_SIZE_X + tid;

	//uint maxGroupCnt = ScanResults[maxParticles - 1].groupID + SortedParticleHashes[maxParticles - 1].flag;

	//uint groupId = ScanResults[globalIndex].groupID;

	/*if (groupId >= maxGroupCnt)
		return;*/

	if (SortedParticleHashes[globalIndex].flag == 1)
	{
		CompactCells[SortedParticleHashes[globalIndex].cellIndex].startIndex = globalIndex;
	}

	if (globalIndex != numParticles - 1 &&
		SortedParticleHashes[globalIndex].cellIndex != SortedParticleHashes[globalIndex + 1].cellIndex)
		CompactCells[SortedParticleHashes[globalIndex].cellIndex].endIndex = globalIndex + 1;

	if (globalIndex == numParticles - 1)
		CompactCells[SortedParticleHashes[globalIndex].cellIndex].endIndex = numParticles;


	//CellMap[CompactCells[groupId].cellKey] = groupId;
}