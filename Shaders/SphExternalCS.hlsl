#include "SphCommon.hlsli"

StructuredBuffer<float3> positions : register(t0);
StructuredBuffer<float3> velocities : register(t2);
StructuredBuffer<float> spawnTimes : register(t6);

RWStructuredBuffer<float3> predPositions : register(u1);
RWStructuredBuffer<float3> predVelocities : register(u3);

[numthreads(GROUP_SIZE_X, 1, 1)]
void main(uint tid : SV_GroupThreadID,
	uint3 gtid : SV_DispatchThreadID,
	uint groupIdx : SV_GroupID)
{
	uint index = groupIdx.x * GROUP_SIZE_X + tid;
	if (index >= numParticles) return;

	float3 pos = positions[index];
	float3 vel = velocities[index];

	if (currentTime < spawnTimes[index])
	{
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

	vel += (gravityAcceleration + externalForce) * deltaTime;

	predVelocities[index] = vel;
	predPositions[index] = pos + vel * deltaTime;
}