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

	if (currentTime < p_i.spawnTime)
	{
		ParticlesOutput[index] = p_i;
		return;
	}

	float3 gravityAcceleration = float3(0, -9.8, 0) * gravityCoeff;
	
	float3 externalForce = float3(0, 0, 0);
	if (forceKey == 10) {
		float t = currentTime;
		if (t >= startTime && t < startTime + duration)
		{
			externalForce.x = -5.0;
		}
	}

	p_i.velocity += (gravityAcceleration + externalForce) * deltaTime;
	p_i.predictedPosition = p_i.position + p_i.velocity * deltaTime;

	ParticlesOutput[index] = p_i;
}