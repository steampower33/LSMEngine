#include "SphCommon.hlsli"

StructuredBuffer<Particle> ParticlesInput : register(t0);
StructuredBuffer<ParticleHash> SortedHashes : register(t1);
StructuredBuffer<CompactCell> CompactCells : register(t2);
StructuredBuffer<int> CellMap : register(t3);

RWStructuredBuffer<Particle> ParticlesOutput : register(u0);

[numthreads(GROUP_SIZE_X, 1, 1)]
void main(uint tid : SV_GroupThreadID,
	uint3 gtid : SV_DispatchThreadID,
	uint groupIdx : SV_GroupID)
{
	uint index = groupIdx.x * GROUP_SIZE_X + tid;

	if (index >= maxParticles) return;

	Particle p_i = ParticlesInput[index];

	float3 pressureForce = float3(0.0, 0.0, 0.0);
	float3 viscosityForce = float3(0.0, 0.0, 0.0);
	float3 gravityForce = float3(0.0, -9.8f, 0.0) * mass * m_gravityCoeff;
	float3 externalForce = float3(0.0, 0.0, 0.0);

	int3 cellID = floor((p_i.position - minBounds) / smoothingRadius);

	for (int i = -1; i <= 1; ++i)
	{
		for (int j = -1; j <= 1; ++j)
		{
			int3 neighborIndex = cellID + int3(i, j, 0);

			uint flatNeighborIndex = GetCellKeyFromCellID(neighborIndex);

			uint startIndex = CompactCells[flatNeighborIndex].startIndex;
			uint endIndex = CompactCells[flatNeighborIndex].endIndex;
			
			if (startIndex == 0xFFFFFFFF || endIndex == 0xFFFFFFFF) continue;

			for (int k = startIndex; k < endIndex; ++k)
			{
				uint particleIndexB = SortedHashes[k].particleID;
				
				//자기자신 제외
				if (index == particleIndexB) continue;
				
				Particle p_j = ParticlesInput[particleIndexB];

				float3 x_ij = p_j.position - p_i.position;
				float dist = length(x_ij);

				if (dist < 1e-9f) continue;

				float3 dir = x_ij / dist;

				pressureForce += -dir * mass * (p_i.pressure + p_j.pressure) / (2.0 * p_j.density) *
					SpikyGradient_2D(dist, smoothingRadius);

				viscosityForce += viscosity * mass * (p_j.velocity - p_i.velocity) / p_j.density *
					ViscosityLaplacian_2D(dist, smoothingRadius);

				//if (SortedHashes[k + 1].cellIndex != SortedHashes[k].cellIndex)
				//	break;
			}
		}
	}

	if (forceKey == 1)
	{
		externalForce = float3(-4.0, 0.0, 0.0) * mass;
	}
	if (forceKey == 2)
	{
		externalForce = float3(4.0, 0.0, 0.0) * mass;
	}

	p_i.force = pressureForce
		+ viscosityForce
		+ gravityForce
		+ externalForce;

	ParticlesOutput[index] = p_i;
}