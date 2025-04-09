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

	Particle p = ParticlesInput[index];

	float h = cellSize;

	p.density = 0.0;
	for (int j = 0; j < maxParticles; j++)
	{
		if (index == j) continue;

		Particle p_j = ParticlesInput[j];

		float dist = length(p.position - p_j.position);

		if (dist >= h)
			continue;

		float w_kernel = CubicSpline(dist / h);

		p.density += mass * w_kernel;
	}
	p.pressure = pressureCoeff * (pow(p.density / density0, 7.0f) - 1.0);

	ParticlesOutput[index] = p;
}