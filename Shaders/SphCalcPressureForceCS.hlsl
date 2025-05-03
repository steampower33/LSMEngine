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

	float density_i = p_i.density;
	float near_density_i = p_i.nearDensity;
	float pressure_i = PressureFromDensity(density_i, density0, pressureCoeff);
	float near_pressure_i = NearPressureFromDensity(near_density_i, nearPressureCoeff);

	float3 velocity_i = p_i.velocity;

	float3 pressureForce = float3(0.0, 0.0, 0.0);
	float3 viscosityForce = float3(0.0, 0.0, 0.0);

	float3 pos_pred_i = p_i.predictedPosition;
	int3 cellID = floor((pos_pred_i - minBounds) / smoothingRadius);
	float sqrRadius = smoothingRadius * smoothingRadius;

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

			//자기자신 제외
			if (index == particleIndexB) continue;

			Particle p_j = ParticlesInput[particleIndexB];

			float3 pos_pred_j = p_j.predictedPosition;

			float3 x_ij_pred = pos_pred_j - pos_pred_i;
			float sqrDist = dot(x_ij_pred, x_ij_pred);

			if (sqrDist > sqrRadius) continue;

			float density_j = p_j.density;
			float near_density_j = p_j.nearDensity;

			float pressure_j = PressureFromDensity(density_j, density0, pressureCoeff);
			float near_pressure_j = NearPressureFromDensity(near_density_j, nearPressureCoeff);

			float sharedPressure = (pressure_i + pressure_j) / 2.0f;
			float sharedNearPressure = (near_pressure_i + near_pressure_j) / 2.0f;

			float3 velocity_j = p_j.velocity;

			float r = length(x_ij_pred);
			float3 dir = r > 0 ? x_ij_pred / r : float3(0.0, 1.0, 0.0);

			pressureForce += dir * mass *
				(sharedPressure / density_j * DensityDerivative(r, smoothingRadius) +
				sharedNearPressure / near_density_j * NearDensityDerivative(r, smoothingRadius));

			viscosityForce += mass * (velocity_j - velocity_i) / density_j * ViscosityLaplacian(r, smoothingRadius);
		}
	}
	float3 acceleration = (pressureForce + viscosity * viscosityForce) / density_i;
	p_i.velocity += acceleration * deltaTime;

	ParticlesOutput[index] = p_i;
}