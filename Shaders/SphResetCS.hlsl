#include "SphCommon.hlsli"

RWStructuredBuffer<float3> Positions : register(u0);
RWStructuredBuffer<float3> Velocities : register(u2);
RWStructuredBuffer<float3> SpawnTimes : register(u6);

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

            float midX = 0; // dam은 좌우 대칭이니까 center Y,Z만 필요
            float midY = 0;
            float midZ = 0;
            // CPU 로직에서 mid 계산과 동일하게 해주시면 됩니다.
            float spacingX = nX * dp;
            float spacingY = nY * dp;
            float spacingZ = nZ * dp;
            float baseX = -maxBounds.x + dp * 10.0;
            float baseY = (-maxBounds.y + midY * 0.5 + dp * 10.0);
            float baseZ = -maxBounds.z + dp;

            Positions[index] = float3(
                baseX + dp * x,
                baseY + dp * y,
                baseZ + dp * z
                );
        }
        else if (index < slice * 2)
        {
            uint idx = index - slice;
            uint z = idx / (nX * nY);
            uint rem = idx % (nX * nY);
            uint y = rem / nX;
            uint x = rem % nX;

            float midX = 0;
            float midY = 0;
            float midZ = 0;
            float baseX = maxBounds.x - dp * 10.0;
            float baseY = (-maxBounds.y + midY * 0.5 + dp * 10.0);
            float baseZ = maxBounds.z - dp;

            Positions[index] = float3(
                baseX - dp * x,
                baseY + dp * y,
                baseZ - dp * z
                );
        }
        else
        {
            Positions[index] = float3(0, 0, 0);
        }

        Velocities[index] = float3(0, 0, 0);
        SpawnTimes[index] = -1.0;
	}
}