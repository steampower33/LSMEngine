#include "SphCommon.hlsli"

StructuredBuffer<Particle> ParticlesInput : register(t0);
//StructuredBuffer<ParticleHash> ParticlesHashes : register(t1);

RWStructuredBuffer<Particle> ParticlesOutput : register(u0);

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

	p.currentAcceleration = p.force / p.density;
	p.velocity = p.velocityHalfStep + p.currentAcceleration * (deltaTime / 2.0f);

	if (p.position.x - smoothingRadius <= minBounds.x && p.velocity.x < 0.0)
	{
		p.velocity.x *= -collisionDamping;
		p.position.x = minBounds.x + smoothingRadius;
	}
	else if (p.position.x + smoothingRadius >= maxBounds.x && p.velocity.x > 0.0)
	{
		p.velocity.x *= -collisionDamping;
		p.position.x = maxBounds.x - smoothingRadius;
	}

	if (p.position.y - smoothingRadius <= minBounds.y && p.velocity.y < 0.0)
	{
		p.velocity.y *= -collisionDamping;
		p.position.y = minBounds.y + smoothingRadius;
	}
	else if (p.position.y + smoothingRadius >= maxBounds.y && p.velocity.y > 0.0)
	{
		p.velocity.y *= -collisionDamping;
		p.position.y = maxBounds.y - smoothingRadius;
	}

	float speed = length(p.velocity);
	float speedT = saturate(speed / 1.0);
	p.color = float3(speedT, 1.0 - speedT, 0.0);

	p.life -= deltaTime;

	// ¾²±â
	ParticlesOutput[index] = p;
}