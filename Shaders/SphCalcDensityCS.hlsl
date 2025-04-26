#include "SphCommon.hlsli"

StructuredBuffer<Particle> ParticlesInput : register(t0);
StructuredBuffer<uint> CellStart : register(t1);
StructuredBuffer<uint> CellCount : register(t2);
StructuredBuffer<uint> SortedIdx : register(t3);

RWStructuredBuffer<Particle> ParticlesOutput : register(u0);

[numthreads(GROUP_SIZE_X, 1, 1)]
void main(uint tid : SV_GroupThreadID,
	uint3 gtid : SV_DispatchThreadID,
	uint groupIdx : SV_GroupID)
{
	uint index = groupIdx.x * GROUP_SIZE_X + tid;
	if (index >= numParticles) return;

	Particle p_i = ParticlesInput[index];

	float t0 = p_i.spawnTime;
	if (currentTime < t0)
		return;

	if (p_i.isGhost)
		return;

	int3 cellID = floor((p_i.position - minBounds) / smoothingRadius);

	p_i.density = 0.0;
	for (int i = 0; i < 27; ++i)
	{
		int3 neighborIndex = cellID + offsets3D[i];

		if ((neighborIndex.x < 0 || neighborIndex.x >= gridDimX) ||
			(neighborIndex.y < 0 || neighborIndex.y >= gridDimY) ||
			(neighborIndex.z < 0 || neighborIndex.z >= gridDimZ))
			continue;

		uint flatNeighborIndex = GetCellKeyFromCellID(neighborIndex);

		uint startIndex = CellStart[flatNeighborIndex];
		uint endIndex = startIndex + CellCount[flatNeighborIndex];

		if (startIndex == endIndex) continue;

		for (int n = startIndex; n < endIndex; ++n)
		{
			uint particleIndexB = SortedIdx[n];

			// Here you can load particleB and do the SPH evaluation logic
			Particle p_j = ParticlesInput[particleIndexB];

			float dist = length(p_j.position - p_i.position);

			float influence = Poly6_3D(dist, smoothingRadius);

			p_i.density += mass * influence;
		}
	}

	p_i.pressure = pressureCoeff * (p_i.density - density0);

	ParticlesOutput[index] = p_i;
}