#include "SphCommon.hlsli"

StructuredBuffer<ParticleHash> ParticlesHashes : register(t0);

RWStructuredBuffer<Particle> Particles : register(u0);

// ������ ���� �ؽ� �Լ� (����� [0, 1] ������ float�� ��ȯ)
float random(uint seed)
{
    // ������ �ؽ� �˰���
    seed = seed * 747796405u + 2891336453u;
    uint result = ((seed >> ((seed >> 28u) + 4u)) ^ seed) * 277803737u;
    result = (result >> 22u) ^ result;
    // ����� [0, 1] ������ float�� ����ȭ
    return float(result) / 4294967295.0f; // 2^32 - 1 �� ����
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

    Particle p = Particles[index];
    float halfSize = p.radius;

    ParticleHash hashInfo = ParticlesHashes[index];

    // ���� �ֱ� Ȯ�� �� ��ƼŬ ������Ʈ ����
    p.velocity += (p.force / mass) * deltaTime;
    p.position += p.velocity * deltaTime;

    p.velocity += float3(0.0, -9.8, 0.0) * deltaTime;

    // ��� ó��
    float epsilon = 0.001f;
    if (p.position.x - halfSize < minBounds.x && p.velocity.x < 0.0)
    {
        p.velocity.x *= -COR;
        p.position.x = minBounds.x + halfSize + epsilon;
    }
    else if (p.position.x + halfSize > maxBounds.x && p.velocity.x > 0.0)
    {
        p.velocity.x *= -COR;
        p.position.x = maxBounds.x - halfSize + epsilon;
    }

    if (p.position.y - halfSize < minBounds.y && p.velocity.y < 0.0)
    {
        p.velocity.y *= -COR;
        p.position.y = minBounds.y + halfSize + epsilon;
    }
    else if (p.position.y + halfSize > maxBounds.y && p.velocity.y > 0.0)
    {
        p.velocity.y *= -COR;
        p.position.y = maxBounds.y - halfSize + epsilon;
    }

    // ���� �ֱ� ����
    //p.life -= deltaTime; // 1.0�� 1�ʸ� �ǹ��Ѵٰ� ����

    //if ((hashInfo.hashValue % 2) == 0) {
    //    p.color = float3(1.0, 0.0, 0.0); // Red if even
    //}
    //else {
    //    p.color = float3(0.0, 1.0, 0.0); // Green if odd
    //}

    // ����
    Particles[index] = p;
}