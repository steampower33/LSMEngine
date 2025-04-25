#include "SphCommon.hlsli"

StructuredBuffer<Particle> ParticlesInput : register(t0);

RWStructuredBuffer<Particle> ParticlesOutput : register(u0);

[numthreads(GROUP_SIZE_X, 1, 1)]
void main(uint tid : SV_GroupThreadID,
	uint3 gtid : SV_DispatchThreadID,
	uint groupIdx : SV_GroupID)
{
	uint index = groupIdx.x * GROUP_SIZE_X + tid;
	if (index >= numParticles) return;

	Particle p_i = ParticlesInput[index];

	//p_i.velocityHalf = p_i.velocity + 0.5 * p_i.currentAcceleration * deltaTime;
	//p_i.position += p_i.velocityHalf * deltaTime;

	ParticlesOutput[index] = p_i;
}