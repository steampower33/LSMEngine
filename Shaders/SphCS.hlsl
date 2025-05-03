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

	Particle p_i = ParticlesInput[index];

	if (currentTime < p_i.spawnTime)
	{
		ParticlesOutput[index] = p_i;
		return;
	}

	float3 pos = p_i.position;
	float3 vel = p_i.velocity;
	float3 Fb = float3(0, 0, 0);

	// X- 면
	float pen = (minBounds.x + radius) - pos.x;
	if (pen > 0)
	{
		Fb.x += boundaryStiffness * pen;
		vel.x -= boundaryDamping * vel.x;
	}
	// X+ 면
	pen = pos.x - (maxBounds.x - radius);
	if (pen > 0)
	{
		Fb.x -= boundaryStiffness * pen;
		vel.x -= boundaryDamping * vel.x;
	}

	// Y- 면
	pen = (minBounds.y + radius) - pos.y;
	if (pen > 0)
	{
		Fb.y += boundaryStiffness * pen;
		vel.y -= boundaryDamping * vel.y;
	}
	// Y+ 면
	pen = pos.y - (maxBounds.y - radius);
	if (pen > 0)
	{
		Fb.y -= 5000.0f * pen;
		vel.y -= 2.0f * vel.y;
	}

	// Z- 면
	pen = (minBounds.z + radius) - pos.z;
	if (pen > 0)
	{
		Fb.z += boundaryStiffness * pen;
		vel.z -= boundaryDamping * vel.z;
	}
	// Z+ 면
	pen = pos.z - (maxBounds.z - radius);
	if (pen > 0)
	{
		Fb.z -= boundaryStiffness * pen;
		vel.z -= boundaryDamping * vel.z;
	}

	p_i.velocity = vel + (Fb / mass) * deltaTime;
	p_i.position += p_i.velocity * deltaTime;

	ParticlesOutput[index] = p_i;
}