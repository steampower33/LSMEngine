#include "SphCommon.hlsli"

StructuredBuffer<Particle> ParticlesInput : register(t0);

RWStructuredBuffer<Particle> ParticlesOutput : register(u0);
//RWStructuredBuffer<float3> ParticlePosition : register(u1);

[numthreads(GROUP_SIZE_X, 1, 1)]
void main(uint tid : SV_GroupThreadID,
	uint3 gtid : SV_DispatchThreadID,
	uint groupIdx : SV_GroupID)
{
	uint index = groupIdx.x * GROUP_SIZE_X + tid;

	if (index >= numParticles) {
		return;
	}

	Particle p = ParticlesInput[index];

	float t0 = p.spawnTime;
	if (currentTime < t0)
		return;

	p.position += p.velocity * deltaTime;

	if (p.position.x - radius <= minBounds.x)
	{
		p.velocity.x *= -collisionDamping;
		p.position.x = minBounds.x + radius;
	}
	else if (p.position.x + radius >= maxBounds.x)
	{
		p.velocity.x *= -collisionDamping;
		p.position.x = maxBounds.x - radius;
	}

	if (p.position.y - radius <= minBounds.y)
	{
		p.velocity.y *= -collisionDamping;
		p.position.y = minBounds.y + radius;
	}
	else if (p.position.y + radius >= maxBounds.y)
	{
		p.velocity.y *= -collisionDamping;
		p.position.y = maxBounds.y - radius;
	}

	if (p.position.z - radius <= minBounds.z)
	{
		p.velocity.z *= -collisionDamping;
		p.position.z = minBounds.z + radius;
	}
	else if (p.position.z + radius >= maxBounds.z)
	{
		p.velocity.z *= -collisionDamping;
		p.position.z = maxBounds.z - radius;
	}

	ParticlesOutput[index] = p;
}