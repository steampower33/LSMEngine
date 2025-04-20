#include "SphCommon.hlsli"

StructuredBuffer<Particle> ParticlesInput : register(t0);

RWStructuredBuffer<Particle> ParticlesOutput : register(u0);
RWStructuredBuffer<uint> CellStart : register(u1);
RWStructuredBuffer<uint> CellCount : register(u2);
RWStructuredBuffer<uint> SortedIdx : register(u3);

[numthreads(GROUP_SIZE_X, 1, 1)]
void main(uint tid : SV_GroupThreadID,
	uint3 gtid : SV_DispatchThreadID,
	uint groupIdx : SV_GroupID)
{
	uint index = groupIdx.x * GROUP_SIZE_X + tid;
	if (index >= numParticles) return;

	Particle p_i = ParticlesInput[index];

	float3 fx = (p_i.position - minBounds) / smoothingRadius;
	fx = max(fx, 0.0);
	int3 cellID = int3(floor(fx));
	cellID = clamp(cellID, int3(0, 0, 0), int3(gridDimX, gridDimY, gridDimZ) - int3(1, 1, 1));

	float density = 0.0;
	for (int i = 0; i < 27; ++i)
	{
		int3 neighborIndex = cellID + offsets3D[i];

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

			density += mass * influence;
		}

	}

	float pressure = pressureCoeff * (p_i.density - density0);

	p_i.density = density;
	p_i.pressure = pressure;
	ParticlesOutput[index] = p_i;
}