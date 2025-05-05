#include "SphCommon.hlsli"

StructuredBuffer<float3> PredictedPositions : register(t1);
StructuredBuffer<float3> PredictedVelocities : register(t3);
StructuredBuffer<float> Densities : register(t4);
StructuredBuffer<float> NearDensities : register(t5);
StructuredBuffer<uint> CellStart : register(t9);
StructuredBuffer<uint> CellCount : register(t7);
StructuredBuffer<uint> SortedIdx : register(t11);

StructuredBuffer<float> spawnTimes : register(t6);

RWStructuredBuffer<float3> Velocities : register(u2);

[numthreads(GROUP_SIZE_X, 1, 1)]
void main(uint tid : SV_GroupThreadID,
	uint3 gtid : SV_DispatchThreadID,
	uint groupIdx : SV_GroupID)
{
	uint index = groupIdx.x * GROUP_SIZE_X + tid;

	if (index >= numParticles) return;

	if (currentTime < spawnTimes[index])
	{
		return;
	}

	float density_i = Densities[index];
	float near_density_i = NearDensities[index];
	float pressure_i = PressureFromDensity(density_i, density0, pressureCoeff);
	float near_pressure_i = NearPressureFromDensity(near_density_i, nearPressureCoeff);

	float3 vel_pred_i = PredictedVelocities[index];

	float3 pressureForce = float3(0.0, 0.0, 0.0);
	float3 viscosityForce = float3(0.0, 0.0, 0.0);

	float3 pos_pred_i = PredictedPositions[index];
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
			uint j = SortedIdx[n];

			//자기자신 제외
			if (index == j) continue;

			float3 pos_pred_j = PredictedPositions[j];

			float3 x_ij_pred = pos_pred_j - pos_pred_i;
			float sqrDist = dot(x_ij_pred, x_ij_pred);

			if (sqrDist > sqrRadius) continue;

			float density_j = Densities[j];
			float near_density_j = NearDensities[j];

			float pressure_j = PressureFromDensity(density_j, density0, pressureCoeff);
			float near_pressure_j = NearPressureFromDensity(near_density_j, nearPressureCoeff);

			float sharedPressure = (pressure_i + pressure_j) / 2.0f;
			float sharedNearPressure = (near_pressure_i + near_pressure_j) / 2.0f;

			float3 vel_pred_j = PredictedVelocities[j];

			float r = length(x_ij_pred);
			float3 dir = r > 0 ? x_ij_pred / r : float3(0.0, 1.0, 0.0);

			pressureForce += dir * mass *
				(sharedPressure / density_j * DensityDerivative(r, smoothingRadius) +
				sharedNearPressure / near_density_j * NearDensityDerivative(r, smoothingRadius));

			viscosityForce += mass * (vel_pred_j - vel_pred_i) / density_j * ViscosityLaplacian(r, smoothingRadius);
		}
	}
	float3 acceleration = (pressureForce + viscosity * viscosityForce) / density_i;

	Velocities[index] = vel_pred_i + acceleration * deltaTime;
}