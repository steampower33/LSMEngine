#include "SphCommon.hlsli"

StructuredBuffer<Particle> ParticlesInput : register(t0);
//StructuredBuffer<ParticleHash> SortedHashes : register(t1);
//StructuredBuffer<CompactCell> CompactCells : register(t2);
//StructuredBuffer<int> CellMap : register(t3);

RWStructuredBuffer<Particle> ParticlesOutput : register(u0);

[numthreads(GROUP_SIZE_X, 1, 1)]
void main(uint tid : SV_GroupThreadID,
	uint3 gtid : SV_DispatchThreadID,
	uint groupIdx : SV_GroupID)
{
	uint index = groupIdx.x * GROUP_SIZE_X + tid;

	if (index >= maxParticles) return;

	Particle p_i = ParticlesInput[index];

	float h = cellSize;

	float3 pressureForce = float3(0.0, 0.0, 0.0);
	float3 viscosityForce = float3(0.0, 0.0, 0.0);
	float3 externalForce = float3(0.0, -9.8f * mass, 0.0);

	float rho_i = p_i.density;
	float pressure_i = p_i.pressure;
	float3 pos_i = p_i.position;
	float3 vel_i = p_i.velocity;

	for (int j = 0; j < maxParticles; j++)
	{
		// 자기자신 제외
		if (index == j) continue;

		Particle p_j = ParticlesInput[j];

		float rho_j = p_j.density;
		float pressure_j = p_j.pressure;
		float3 pos_j = p_j.position;
		float3 vel_j = p_j.velocity;
		float3 x_ij = pos_i - pos_j;

		float dist = length(x_ij);

		if (dist >= h)
			continue;

		if (dist < 1e-3f)
			continue;

		float w_kernel = CubicSplineGrad(dist / h);

		float3 gradPressure =
			rho_i * mass *
			(pressure_i / (rho_i * rho_i) + pressure_j / (rho_j * rho_j)) *
			w_kernel * x_ij / dist;
		float3 laplacianVelocity =
			2.0 * mass / rho_j * (vel_i - vel_j) /
			(dist * dist + 0.01f * h * h) *
			w_kernel * dot(x_ij, x_ij / dist);

		pressureForce -= mass / rho_i * gradPressure;
		viscosityForce += mass * viscosity * laplacianVelocity;
	}

	p_i.force = pressureForce + viscosityForce + externalForce;

	ParticlesOutput[index] = p_i;
}