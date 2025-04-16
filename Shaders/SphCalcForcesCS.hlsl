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
	float3 gravityForce = float3(0.0, -9.8f, 0.0) * mass * gravity;
	float3 externalForce = float3(0.0, 0.0, 0.0);

	uint3 cellID = (p_i.position - minBounds) / smoothingRadius;

	for (int offsetY = -1; offsetY <= 1; offsetY++)
	{
		for (int offsetX = -1; offsetX <= 1; offsetX++)
		{
			uint3 offsetCellId = cellID + uint3(offsetX, offsetY, 0);

			if ((offsetCellId.x >= 0 && offsetCellId.x < gridDimX) &&
				(offsetCellId.y >= 0 && offsetCellId.y < gridDimY))
			{
				uint cellKey = GetCellKeyFromCellID(offsetCellId);

				int groupID = CellMap[cellKey];

				CompactCell cell = CompactCells[groupID];

				for (int j = cell.startIndex; j < cell.endIndex; j++)
				{
					 //자기자신 제외
					if (index == j) continue;

					Particle p_j = ParticlesInput[SortedHashes[j].particleID];

					float3 x_ij = p_j.position - p_i.position;
					float dist = length(x_ij);

					if (dist < 1e-9f) continue;

					float3 dir = x_ij / dist;

					pressureForce += -dir * mass * (p_i.pressure + p_j.pressure) / (2.0 * p_j.density) *
						SpikyGradient_2D(dist, smoothingRadius);

					viscosityForce += viscosity * mass * (p_j.velocity - p_i.velocity) / p_j.density * 
						ViscosityLaplacian_2D(dist, smoothingRadius);
				}
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
		+ gravityForce * gravity
		+ externalForce;

	ParticlesOutput[index] = p_i;
}