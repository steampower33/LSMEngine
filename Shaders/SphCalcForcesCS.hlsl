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

	float3 pressureForce = float3(0.0, 0.0, 0.0);
	float3 viscosityForce = float3(0.0, 0.0, 0.0);
	float3 gravityForce = float3(0.0, -9.8f, 0.0) * mass * gravityCoeff;
	float3 externalForce = float3(0.0, 0.0, 0.0);

	int3 cellID = floor((p_i.position - minBounds) / smoothingRadius);

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

			//자기자신 제외
			if (index == particleIndexB) continue;

			Particle p_j = ParticlesInput[particleIndexB];

			float3 x_ij = p_j.position - p_i.position;
			float dist = length(x_ij);

			if (dist < 1e-9f) continue;

			float3 dir = x_ij / dist;

			if (!p_j.isGhost)
			{
				pressureForce += -dir * mass * (p_i.pressure + p_j.pressure) / (2.0 * p_j.density) *
					SpikyGradient_3D(dist, smoothingRadius);

				viscosityForce += viscosity * mass * (p_j.velocity - p_i.velocity) / p_j.density *
					ViscosityLaplacian_3D(dist, smoothingRadius);
			}
			else
			{
				pressureForce += -dir * mass * (p_i.pressure) / (2.0 * p_j.density) *
					SpikyGradient_3D(dist, smoothingRadius);

				viscosityForce += viscosity * mass * (0.0 - p_i.velocity) / p_j.density *
					ViscosityLaplacian_3D(dist, smoothingRadius);
			}
		}
	}

	if (forceKey == 1)
	{
		externalForce += float3(-5.0, 0.0, 0.0);
	}
	else if (forceKey == 2)
	{
		externalForce += float3(5.0, 0.0, 0.0);
	}

	p_i.force = pressureForce
		+ viscosityForce
		+ gravityForce
		+ externalForce;

	ParticlesOutput[index] = p_i;
}