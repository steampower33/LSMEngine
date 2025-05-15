#include "SphCommon.hlsli"

RWStructuredBuffer<float3> Positions : register(u0);
RWStructuredBuffer<float3> Velocities : register(u2);
RWStructuredBuffer<float> SpawnTimes : register(u6);

[numthreads(GROUP_SIZE_X, 1, 1)]
void main(uint tid : SV_GroupThreadID,
	uint3 gtid : SV_DispatchThreadID,
	uint groupIdx : SV_GroupID)
{
	uint index = groupIdx.x * GROUP_SIZE_X + tid;

	if (index >= numParticles) return;

	if (reset == 1)
	{
		uint slice = nX * nY * nZ;
		// lower wall
		if (index < slice)
		{
			uint idx = index;
			uint z = idx / (nX * nY);
			uint rem = idx % (nX * nY);
			uint y = rem / nX;
			uint x = rem % nX;

			Positions[index] = float3(
				-maxBounds.x + dp * 10.0 + dp * x,
				-maxBounds.y + dp * 10.0 + dp * y,
				-maxBounds.z + dp + dp * z
				);
		}
		else
		{
			uint idx = index - slice;
			uint z = idx / (nX * nY);
			uint rem = idx % (nX * nY);
			uint y = rem / nX;
			uint x = rem % nX;

			Positions[index] = float3(
				maxBounds.x - dp * 10.0 - dp * x,
				-maxBounds.y + dp * 10.0 + dp * y,
				maxBounds.z - dp - dp * z
				);
		}

		Velocities[index] = float3(0, 0, 0);
		SpawnTimes[index] = -1.0;
	}
	else if (reset == 2)
	{
		const uint num1 = 8, num2 = 16, num3 = 24, num4 = 32;
		const float radius1 = dp * 3.0f;
		const float radius2 = dp * 6.0f;
		const float radius3 = dp * 9.0f;
		const float radius4 = dp * 12.0f;
		const uint batchSize = 1 + num1 + num2 + num3 + num4;

		float ringSpacing = deltaTime * spawnTimeStep;

		uint group = index / batchSize;
		uint subIdx = index % batchSize;

		SpawnTimes[index] = group * ringSpacing;

		float3 center = emitterPos;
		float3 pos = float3(0.0, 0.0, 0.0);
		if (subIdx == 0)
		{
			Positions[index] = center;
		}
		else if (subIdx < batchSize)
		{
			float radius;
			uint  idx;
			float angle;
			if (subIdx < 1 + num1) {
				idx = subIdx - 1;
				radius = radius1;
				angle = idx * (2 * PI / num1);
			}
			else if (subIdx < 1 + num1 + num2) {
				idx = subIdx - 1 - num1;
				radius = radius2;
				angle = idx * (2 * PI / num2);
			}
			else if (subIdx < 1 + num1 + num2 + num3) {
				idx = subIdx - 1 - num1 - num2;
				radius = radius3;
				angle = idx * (2 * PI / num3);
			}
			else {
				idx = subIdx - 1 - num1 - num2 - num3;
				radius = radius4;
				angle = idx * (2 * PI / num4);
			}

			float3 W = normalize(emitterDir);
			float3 up = abs(dot(W, float3(0, 1, 0))) > 0.99 ? float3(1, 0, 0) : float3(0, 1, 0);
			float3 U = normalize(cross(up, W));
			float3 V = cross(W, U);
			float3 offset = U * (radius * cos(angle))
				          + V * (radius * sin(angle));
			pos = center + offset;
		}

		float3 velDir = normalize(emitterDir);
		float3 vel = velDir * emitterVel;

		Positions[index] = pos;
		Velocities[index] = vel;
	}
}