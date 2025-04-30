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

	float3 pos_pred_i = p_i.predictedPosition;

	int3 cellID = floor((pos_pred_i - minBounds) / smoothingRadius);

	float density = 0.0;
	float nearDensity = 0.0;
	float sqrRadius = smoothingRadius * smoothingRadius;
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

			Particle p_j = ParticlesInput[particleIndexB];

			float3 x_ij_pred = p_j.predictedPosition - pos_pred_i;

			float sqrDist = dot(x_ij_pred, x_ij_pred);

			if (sqrDist > sqrRadius) continue;

			float dist = sqrt(sqrDist);
			density += mass * DensityKernel(dist, smoothingRadius);
			nearDensity += mass * NearDensityKernel(dist, smoothingRadius);
		}
	}

	p_i.density = density;
	p_i.nearDensity = nearDensity;

	ParticlesOutput[index] = p_i;
}