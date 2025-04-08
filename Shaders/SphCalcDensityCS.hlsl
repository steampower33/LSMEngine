#include "SphCommon.hlsli"

StructuredBuffer<ParticleHash> SortedHashes : register(t0);
StructuredBuffer<CompactCell> CompactCells : register(t1);
StructuredBuffer<int> CellMap : register(t2);

RWStructuredBuffer<Particle> Particles : register(u0);

[numthreads(GROUP_SIZE_X, 1, 1)]
void main(uint tid : SV_GroupThreadID,
	uint3 gtid : SV_DispatchThreadID,
	uint groupIdx : SV_GroupID)
{
	uint index = groupIdx.x * GROUP_SIZE_X + tid;
	if (index >= maxParticles) return;

	Particle p = Particles[index];
	float radius = p.radius * 2;

	float density = 0.0;
	for (int j = 0; j < maxParticles; j++)
	{
		if (index == j) continue;

		Particle p_j = Particles[j];

		float dist = length(p.position - p_j.position);

		if (dist >= radius)
			continue;

		density += mass * CubicSpline(dist * 2.0f / radius);
	}
	Particles[index].density = density;
	Particles[index].pressure = pressureCoeff * (pow(density / density0, 7.0f) - 1.0);
}