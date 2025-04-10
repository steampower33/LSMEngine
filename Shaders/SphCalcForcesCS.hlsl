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
	float3 externalForce = float3(0.0, -9.8f * mass, 0.0);

	for (int j = 0; j < maxParticles; j++)
	{
		// 자기자신 제외
		if (index == j) continue;

		Particle p_j = ParticlesInput[j];

		float3 x_ij = p_i.predictedPosition - p_j.position;
		float dist = length(x_ij);

		if (dist < 1e-3f) continue;

		float vRange = 1.0f;
		uint baseSeed = index * 17u;
		float rndVelX = random(baseSeed + 3u);
		float rndVelY = random(baseSeed + 4u);
		/*float3 dir_ij = dist == 0 ? 
			float3(lerp(-vRange, vRange, rndVelX), lerp(-vRange, vRange, rndVelY), 0.0f) :
			x_ij / dist;*/

		float3 dir_ij = x_ij / dist;

		float slope = SmoothingKernelDerivative(dist, smoothingRadius);
		float sharedPressure = (p_i.pressure + p_j.pressure) * 0.5;
		pressureForce -= p_j.pressure * dir_ij * slope * mass / (p_j.density + 1e-3f);
	}
	p_i.force = pressureForce;

	ParticlesOutput[index] = p_i;
}