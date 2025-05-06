#include "SphCommon.hlsli"

StructuredBuffer<float3> PredictedPositions : register(t1);

StructuredBuffer<float> spawnTimes : register(t6);

RWStructuredBuffer<float3> Positions: register(u0);
RWStructuredBuffer<float3> Velocities : register(u2);

RWStructuredBuffer<float3> Colors : register(u12);

[numthreads(GROUP_SIZE_X, 1, 1)]
void main(uint tid : SV_GroupThreadID,
	uint3 gtid : SV_DispatchThreadID,
	uint groupIdx : SV_GroupID)
{
	uint index = groupIdx.x * GROUP_SIZE_X + tid;

	if (index >= numParticles) return;

	if (currentTime < spawnTimes[index]) return;

	float3 pos = PredictedPositions[index];
	float3 vel = Velocities[index];
	float3 Fb = float3(0, 0, 0);

	// X- 면
	float pen = (minBounds.x + radius) - pos.x;
	if (pen > 0)
	{
		Fb.x += boundaryStiffness * pen;
		if (dot(vel, float3(-minBounds.x, 0.0, 0.0)) < 0.0)
			vel.x -= boundaryDamping * vel.x;
	}
	// X+ 면
	pen = pos.x - (maxBounds.x - radius);
	if (pen > 0)
	{
		Fb.x -= boundaryStiffness * pen;
		if (dot(vel, float3(minBounds.x, 0.0, 0.0)) < 0.0)
			vel.x -= boundaryDamping * vel.x;
	}

	// Y- 면
	pen = (minBounds.y + radius) - pos.y;
	if (pen > 0)
	{
		Fb.y += boundaryStiffness * pen;
		if (dot(vel, float3(0.0, -minBounds.y, 0.0)) < 0.0)
			vel.y -= boundaryDamping * vel.y;
	}
	//// Y+ 면
	//pen = pos.y - (maxBounds.y - radius);
	//if (pen > 0)
	//{
	//	Fb.y -= boundaryStiffness * pen;
	//	if (dot(vel, float3(0.0, minBounds.y, 0.0)) < 0.0)
	//		vel.y -= boundaryDamping * vel.y;
	//}

	// Z- 면
	pen = (minBounds.z + radius) - pos.z;
	if (pen > 0)
	{
		Fb.z += boundaryStiffness * pen;
		if (dot(vel, float3(0.0, 0.0, -minBounds.z)) < 0.0)
			vel.z -= boundaryDamping * vel.z;
	}
	// Z+ 면
	pen = pos.z - (maxBounds.z - radius);
	if (pen > 0)
	{
		Fb.z -= boundaryStiffness * pen;
		if (dot(vel, float3(0.0, 0.0, minBounds.z)) < 0.0)
			vel.z -= boundaryDamping * vel.z;
	}

	vel += (Fb / mass) * deltaTime;
	Positions[index] += vel * deltaTime;
	Velocities[index] = vel;

	Colors[index] = float3(1.0, 1.0, 1.0);
}