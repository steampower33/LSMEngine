#include "SphCommon.hlsli"

StructuredBuffer<Particle> ParticlesInput : register(t0);
//StructuredBuffer<ParticleHash> ParticlesHashes : register(t1);

RWStructuredBuffer<Particle> ParticlesOutput : register(u0);

// 간단한 정수 해시 함수 (결과를 [0, 1] 범위의 float로 변환)
float random(uint seed)
{
	seed = seed * 747796405u + 2891336453u;
	uint result = ((seed >> ((seed >> 28u) + 4u)) ^ seed) * 277803737u;
	result = (result >> 22u) ^ result;
	// 결과를 [0, 1] 범위의 float로 정규화
	return float(result) / 4294967295.0f; // 2^32 - 1 로 나눔
}

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
	if (p.life < 0)
	{
		uint baseSeed = index * 17u;

		float rndPosX = random(baseSeed + 1u);
		float rndPosY = random(baseSeed + 2u);
		float rndVelX = random(baseSeed + 3u);
		float rndVelY = random(baseSeed + 4u);
		float rndLife = random(baseSeed + 5u);

		p.position = float3(lerp(minBounds.x, maxBounds.x, rndPosX),
			lerp(minBounds.y, maxBounds.y, rndPosY),
			0.0f);

		float vRange = 1.0f;
		p.velocity = float3(lerp(-vRange, vRange, rndVelX),
			lerp(-vRange, vRange, rndVelY),
			0.0f);

		p.life = 10.0f;
	}
	else
	{
		p.velocity += (p.force / mass) * deltaTime;
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

		//p.life -= deltaTime;

	}
	// 쓰기
	ParticlesOutput[index] = p;
}