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

	if (currentTime < p_i.spawnTime || p_i.isGhost)
	{
		ParticlesOutput[index] = p_i;
		return;
	}

	float3 pos_pred_i = p_i.predictedPosition;
	int3 cellID = floor((pos_pred_i - minBounds) / smoothingRadius);
	float sqrRadius = smoothingRadius * smoothingRadius;

	float3 velocity = p_i.velocity;

	float3 viscosityForce = float3(0.0, 0.0, 0.0);
	for (int i = 0; i < 27; ++i)
	{
		int3 neighborIndex = cellID + offsets3D[i];

		//if ((neighborIndex.x < 0 || neighborIndex.x >= gridDimX) ||
		//	(neighborIndex.y < 0 || neighborIndex.y >= gridDimY) ||
		//	(neighborIndex.z < 0 || neighborIndex.z >= gridDimZ))
		//	continue;

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

			float3 pos_pred_j = p_j.predictedPosition;

			float3 x_ij_pred = pos_pred_j - pos_pred_i;
			float sqrDist = dot(x_ij_pred, x_ij_pred);

			if (sqrDist > sqrRadius) continue;

			float dist = length(x_ij_pred);

			float3 neighbourVelocity = p_j.velocity;

			viscosityForce += (neighbourVelocity - velocity) * SmoothingKernelPoly6(dist, smoothingRadius);
		}
	}

	p_i.velocity += viscosityForce * viscosity * deltaTime;

	ParticlesOutput[index] = p_i;
}