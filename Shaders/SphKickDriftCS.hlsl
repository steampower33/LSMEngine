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

	Particle p = ParticlesInput[index];

	float3 gravityAcceleration = float3(0, -9.8, 0);

	float3 externalForce = float3(0.0, 0.0, 0.0);
	if (forceKey == 1)
	{
		if (p.position.y < (maxBounds.y + minBounds.y))
			externalForce += float3(-5.0, 0.0, 0.0);
	}
	else if (forceKey == 2)
	{
		if (p.position.y < (maxBounds.y + minBounds.y))
			externalForce += float3(5.0, 0.0, 0.0);
	}
	float3 externalAcceleration = externalForce / mass;

	p.velocity += (gravityAcceleration + externalAcceleration) * deltaTime;
	p.predictedPosition = p.position + p.velocity * deltaTime;

	ParticlesOutput[index] = p;
}