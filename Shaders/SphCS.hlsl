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
	//if (p.life < 0)
	//{
	//	uint baseSeed = index * 17u;

	//	float rndPosX = random(baseSeed + 1u);
	//	float rndPosY = random(baseSeed + 2u);
	//	float rndVelX = random(baseSeed + 3u);
	//	float rndVelY = random(baseSeed + 4u);
	//	float rndLife = random(baseSeed + 5u);

	//	/*p.position = float3(lerp(minBounds.x, maxBounds.x, rndPosX),
	//		lerp(minBounds.y, maxBounds.y, rndPosY),
	//		0.0f);*/

	//	p.position = float3(lerp(3.0f, maxBounds.x, rndPosX),
	//		lerp(3.0, maxBounds.y, rndPosY),
	//		0.0f);

	//	float vRange = 1.0f;
	//	//lerp(-vRange, vRange, rndVelX)
	//	p.velocity = float3(-1.0f,
	//		lerp(-vRange, vRange, rndVelY),
	//		0.0f);

	//	p.life = lerp(0, 20.0, rndLife);
	//}
	//else
	//{

	//}

	p.velocity += p.force / (p.density + 1e-3f) * deltaTime;
	p.position += p.velocity * deltaTime;

	float radius = p.radius;
	float epsilon = 1e-3f;
	if (p.position.x - radius < minBounds.x && p.velocity.x < 0.0)
	{
		p.velocity.x *= -COR;
		p.position.x = minBounds.x + radius + epsilon;
	}
	else if (p.position.x + radius > maxBounds.x && p.velocity.x > 0.0)
	{
		p.velocity.x *= -COR;
		p.position.x = maxBounds.x - radius + epsilon;
	}

	if (p.position.y - radius < minBounds.y && p.velocity.y < 0.0)
	{
		p.velocity.y *= -COR;
		p.position.y = minBounds.y + radius + epsilon;
	}
	else if (p.position.y + radius > maxBounds.y && p.velocity.y > 0.0)
	{
		p.velocity.y *= -COR;
		p.position.y = maxBounds.y - radius + epsilon;
	}

	float speed = length(p.velocity);
	float speedT = saturate(speed / 1.0);
	p.color = float3(speedT, 1.0 - speedT, 0.0);

	p.life -= deltaTime;

	// ¾²±â
	ParticlesOutput[index] = p;
}