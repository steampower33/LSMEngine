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

	//p_i.velocityHalfStep = p_i.velocity + p_i.currentAcceleration * (deltaTime / 2.0f);
	//p_i.position += p_i.velocityHalfStep * deltaTime;
	//p_i.predictedPosition = p_i.position + p_i.velocity * deltaTime;

	p_i.density = 0.0;
	for (int j = 0; j < maxParticles; j++)
	{
		Particle p_j = ParticlesInput[j];

		float dist = length(p_j.position - p_i.position);

		float influence = Poly6(dist, smoothingRadius);

		p_i.density += mass * influence;
	}

	p_i.pressure = pressureCoeff * (p_i.density - density0);

	ParticlesOutput[index] = p_i;
}