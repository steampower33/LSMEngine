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

	int3 cellID = floor((p_i.position - minBounds) / smoothingRadius);

	p_i.density = 0.0;
	for (int i = -1; i <= 1; ++i)
	{
		for (int j = -1; j <= 1; ++j)
		{
			int3 neighborIndex = cellID + int3(i, j, 0);

			uint flatNeighborIndex = GetCellKeyFromCellID(neighborIndex);

			uint startIndex = CompactCells[flatNeighborIndex].startIndex;
			uint endIndex = CompactCells[flatNeighborIndex].endIndex;

			if (startIndex == 0xFFFFFFFF || endIndex == 0xFFFFFFFF) continue;

			for (int k = startIndex; k < endIndex; ++k)
			{
				uint particleIndexB = SortedHashes[k].particleID;

				// Here you can load particleB and do the SPH evaluation logic
				Particle p_j = ParticlesInput[particleIndexB];

				float dist = length(p_j.position - p_i.position);

				float influence = Poly6_2D(dist, smoothingRadius);

				p_i.density += mass * influence;
			
				//if (k != numParticles - 1 && SortedHashes[k + 1].cellIndex != SortedHashes[k].cellIndex)
				//	break;
			}

		}
	}

	p_i.density += 1e-9f;

	//for (int j = 0; j < maxParticles; j++)
	//{
	//	Particle p_j = ParticlesInput[j];

	//	float dist = length(p_j.position - p_i.position);

	//	float influence = Poly6_2D(dist, smoothingRadius);

	//	p_i.density += mass * influence;
	//}

	p_i.pressure = pressureCoeff * (p_i.density - density0);

	ParticlesOutput[index] = p_i;
}