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

}