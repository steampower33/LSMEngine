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

	//if (p.isGhost)
	//	return;

	//p.currentAcceleration = p.force / mass;
	//p.velocity = p.velocityHalf + 0.5 * p.currentAcceleration * deltaTime;

	p.velocity += p.force / mass * deltaTime;
	p.position += p.velocity * deltaTime;

	//if (p.position.x - p.radius <= minBounds.x)
	//{
	//	p.velocity.x *= -collisionDamping;
	//	p.position.x = minBounds.x + p.radius;
	//}
	//else if (p.position.x + p.radius >= maxBounds.x)
	//{
	//	p.velocity.x *= -collisionDamping;
	//	p.position.x = maxBounds.x - p.radius;
	//}

	//if (p.position.y - p.radius <= minBounds.y)
	//{
	//	p.velocity.y *= -collisionDamping;
	//	p.position.y = minBounds.y + p.radius;
	//}
	//else if (p.position.y + p.radius >= maxBounds.y)
	//{
	//	p.velocity.y *= -collisionDamping;
	//	p.position.y = maxBounds.y - p.radius;
	//}

	//if (p.position.z - p.radius <= minBounds.z)
	//{
	//	p.velocity.z *= -collisionDamping;
	//	p.position.z = minBounds.z + p.radius;
	//}
	//else if (p.position.z + p.radius >= maxBounds.z)
	//{
	//	p.velocity.z *= -collisionDamping;
	//	p.position.z = maxBounds.z - p.radius;
	//}

	ParticlesOutput[index] = p;
}