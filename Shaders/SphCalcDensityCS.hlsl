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

	p_i.predictedPosition = p_i.position + p_i.velocity * deltaTime;

	p_i.density = 0.0;
	for (int j = 0; j < maxParticles; j++)
	{
		if (index == j) continue;

		Particle p_j = ParticlesInput[j];

		float dist = length(p_i.predictedPosition - p_j.position);

		if (dist < 1e-3f) continue;

		float influence = SmoothingKernel(dist, smoothingRadius);

		p_i.density += mass * influence;
	}
	p_i.pressure = (p_i.density - density0) * pressureCoeff;

	ParticlesOutput[index] = p_i;
}