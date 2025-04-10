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

	float3 pressureForce = float3(0.0, 0.0, 0.0);
	float3 viscosityForce = float3(0.0, 0.0, 0.0);
	float3 gravityForce = float3(0.0, 0.0, 0.0);

	for (int j = 0; j < maxParticles; j++)
	{
		// 자기자신 제외
		if (index == j) continue;

		Particle p_j = ParticlesInput[j];

		float3 x_ij = p_j.predictedPosition - p_i.predictedPosition;
		float dist = length(x_ij);

		float vRange = 1.0f;
		uint baseSeed = index * 17u;
		float rndVelX = random(baseSeed + 3u);
		float rndVelY = random(baseSeed + 4u);
		float3 dir = dist == 0 ?
			float3(lerp(-vRange, vRange, rndVelX), lerp(-vRange, vRange, rndVelY), 0.0f) :
			x_ij / dist;

		//float3 dir = x_ij / dist;

		float slope = SmoothingKerenlPressure(dist, smoothingRadius);
		pressureForce += -dir * mass * (p_i.pressure + p_j.pressure) / (2.0 * p_j.density) * 
			SmoothingKerenlPressure(dist, smoothingRadius);

		float kernelLaplacian = SmoothingKernelLaplacian(dist, smoothingRadius);

		viscosityForce += viscosity * mass * (p_j.velocity - p_i.velocity) / p_j.density * kernelLaplacian;
	}
	gravityForce = float3(0.0, -9.8f, 0.0) * mass / p_i.density * gravity;
	p_i.force = pressureForce + viscosityForce + gravityForce;
	//p_i.force = pressureForce;

	ParticlesOutput[index] = p_i;
}