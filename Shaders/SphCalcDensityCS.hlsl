#include "SphCommon.hlsli"

StructuredBuffer<Particle> ParticlesInput : register(t0);
StructuredBuffer<ParticleHash> SortedHashes : register(t1);
StructuredBuffer<CompactCell> CompactCells : register(t2);
StructuredBuffer<int> CellMap : register(t3);

RWStructuredBuffer<Particle> ParticlesOutput : register(u0);

[numthreads(GROUP_SIZE_X, 1, 1)]
void main(uint tid : SV_GroupThreadID,
	uint3 gtid : SV_DispatchThreadID,
	uint groupIdx : SV_GroupID)
{
	uint index = groupIdx.x * GROUP_SIZE_X + tid;
	if (index >= maxParticles) return;

	Particle p_i = ParticlesInput[index];

	//p_i.velocityHalfStep = p_i.velocity + p_i.currentAcceleration * (deltaTime / 2.0f);
	//p_i.position += p_i.velocityHalfStep * deltaTime;
	//p_i.predictedPosition = p_i.position + p_i.velocity * deltaTime;

	uint3 cellID = (p_i.position - minBounds) / smoothingRadius;

	p_i.density = 0.0;
	for (int offsetY = -1; offsetY <= 1; offsetY++)
	{
		for (int offsetX = -1; offsetX <= 1; offsetX++)
		{
			uint3 offsetCellId = cellID + uint3(offsetX, offsetY, 0);

			if ((offsetCellId.x >= 0 && offsetCellId.x < gridDimX) &&
				(offsetCellId.y >= 0 && offsetCellId.y < gridDimY))
			{
				uint cellKey = GetCellKeyFromCellID(offsetCellId);

				int groupID = CellMap[cellKey];

				if (groupID < 0) continue;

				CompactCell cell = CompactCells[groupID];

				for (int j = cell.startIndex; j < cell.endIndex; j++)
				{
					Particle p_j = ParticlesInput[SortedHashes[j].particleID];

					float dist = length(p_j.position - p_i.position);

					float influence = Poly6_2D(dist, smoothingRadius);

					p_i.density += mass * influence;
				}
			}
		}
	}

	if (p_i.density < 1e-9f)
		p_i.density = 1e-9f;
	p_i.pressure = pressureCoeff * (p_i.density - density0);

	ParticlesOutput[index] = p_i;
}