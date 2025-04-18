#include "SphCommon.hlsli"

StructuredBuffer<Particle> ParticlesInput : register(t0);
StructuredBuffer<ParticleHash> SortedHashes : register(t1);
StructuredBuffer<CompactCell> CompactCells : register(t2);

RWStructuredBuffer<Particle> ParticlesOutput : register(u0);

[numthreads(GROUP_SIZE_X, 1, 1)]
void main(uint tid : SV_GroupThreadID,
	uint3 gtid : SV_DispatchThreadID,
	uint groupIdx : SV_GroupID)
{
	uint index = groupIdx.x * GROUP_SIZE_X + tid;
	if (index >= maxParticles) return;

	Particle p_i = ParticlesInput[index];

	int3 cellID = floor((p_i.position - minBounds) / smoothingRadius);

	p_i.density = 0.0;
	for (int i = 0; i < 27; ++i)
	{
		int3 neighborIndex = cellID + offsets3D[i];

		uint flatNeighborIndex = GetCellKeyFromCellID(neighborIndex);

		uint startIndex = CompactCells[flatNeighborIndex].startIndex;
		uint endIndex = CompactCells[flatNeighborIndex].endIndex;

		if (startIndex == 0xFFFFFFFF || endIndex == 0xFFFFFFFF) continue;

		for (int n = startIndex; n < endIndex; ++n)
		{
			uint particleIndexB = SortedHashes[n].particleID;

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