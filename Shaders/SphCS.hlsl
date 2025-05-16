#include "SphCommon.hlsli"

StructuredBuffer<float3> PredictedPositions : register(t1);

StructuredBuffer<float> spawnTimes : register(t6);

RWStructuredBuffer<float3> Positions: register(u0);
RWStructuredBuffer<float3> Velocities : register(u2);

void CylinderBoundary(in float3 pos, inout float3 Fb, inout float3 vel)
{
	float3 center = float3(0.0, 0.0, 0.0);
	float3 displacement = float3(pos.x - center.x, 0.0, pos.z - center.z);
	float distance = length(displacement);
	float cylinderRadius = maxBounds.x;
	if (distance > cylinderRadius - radius)
	{
		float3 normal = normalize(displacement);
		Fb += normal * boundaryStiffness * (cylinderRadius - radius - distance);
	}

	// Y- 면
	float pen = (minBounds.y + radius) - pos.y;
	if (pen > 0)
	{
		Fb.y += boundaryStiffness * pen;
		if (dot(vel, float3(0.0, 1.0, 0.0)) < 0.0)
			vel.y *= (1.0 - boundaryDamping);
	}
	// Y+ 면
	pen = pos.y - (maxBounds.y - radius);
	if (pen > 0)
	{
		Fb.y -= boundaryStiffness * pen;
		if (dot(vel, float3(0.0, -1.0, 0.0)) < 0.0)
			vel.y -= boundaryDamping * vel.y;
	}
}

void BoxBoundary(in float3 pos, inout float3 Fb, inout float3 vel)
{
	// X- 면
	float pen = (minBounds.x + radius) - pos.x;
	if (pen > 0)
	{
		Fb.x += boundaryStiffness * pen;
		if (dot(vel, float3(1.0, 0.0, 0.0)) < 0.0)
			vel.x *= (1.0 - boundaryDamping);
	}
	// X+ 면
	pen = pos.x - (maxBounds.x - radius);
	if (pen > 0)
	{
		Fb.x -= boundaryStiffness * pen;
		if (dot(vel, float3(-1.0, 0.0, 0.0)) < 0.0)
			vel.x *= (1.0 - boundaryDamping);
	}

	// Y- 면
	pen = (minBounds.y + radius) - pos.y;
	if (pen > 0)
	{
		Fb.y += boundaryStiffness * pen;
		if (dot(vel, float3(0.0, 1.0, 0.0)) < 0.0)
			vel.y *= (1.0 - boundaryDamping);
	}
	// Y+ 면
	pen = pos.y - (maxBounds.y - radius);
	if (pen > 0)
	{
		Fb.y -= boundaryStiffness * pen;
		if (dot(vel, float3(0.0, -1.0, 0.0)) < 0.0)
			vel.y -= boundaryDamping * vel.y;
	}

	// Z- 면
	pen = (minBounds.z + radius) - pos.z;
	if (pen > 0)
	{
		Fb.z += boundaryStiffness * pen;
		if (dot(vel, float3(0.0, 0.0, 1.0)) < 0.0)
			vel.z *= (1.0 - boundaryDamping);
	}
	// Z+ 면
	pen = pos.z - (maxBounds.z - radius);
	if (pen > 0)
	{
		Fb.z -= boundaryStiffness * pen;
		if (dot(vel, float3(0.0, 0.0, -1.0)) < 0.0)
			vel.z *= (1.0 - boundaryDamping);
	}
}

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

	if (boundaryMode == 1)
		BoxBoundary(pos, Fb, vel);
	else if (boundaryMode == 2)
		CylinderBoundary(pos, Fb, vel);

	vel += (Fb / mass) * deltaTime;
	Positions[index] += vel * deltaTime;
	Velocities[index] = vel;
}