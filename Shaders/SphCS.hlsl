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

	if (currentTime < p_i.spawnTime || p_i.isGhost)
	{
		ParticlesOutput[index] = p_i;
		return;
	}

	p_i.position += p_i.velocity * deltaTime;

    float3 pos = p_i.position;
    float3 vel = p_i.velocity;
    bool collided = false;

    // X�� ��� �˻�
    if (pos.x - radius < minBounds.x) // ���� �ݰ� ���
    {
        pos.x = minBounds.x + radius + 1e-5f; // ��� �ٷ� �������� (���Ƿ� �߰�)
        if (vel.x < 0) vel.x *= -collisionDamping; // �������� ���ϴ� �ӵ��� ����/����
        collided = true;
    }
    else if (pos.x + radius > maxBounds.x)
    {
        pos.x = maxBounds.x - radius - 1e-5f;
        if (vel.x > 0) vel.x *= -collisionDamping;
        collided = true;
    }

    // Y�� ��� �˻�
    if (pos.y - radius < minBounds.y)
    {
        pos.y = minBounds.y + radius + 1e-5f;
        if (vel.y < 0) vel.y *= -collisionDamping;
        collided = true;
    }
    else if (pos.y + radius > maxBounds.y)
    {
        pos.y = maxBounds.y - radius - 1e-5f;
        if (vel.y > 0) vel.y *= -collisionDamping;
        collided = true;
    }

    // Z�� ��� �˻�
    if (pos.z - radius < minBounds.z)
    {
        pos.z = minBounds.z + radius + 1e-5f;
        if (vel.z < 0) vel.z *= -collisionDamping;
        collided = true;
    }
    else if (pos.z + radius > maxBounds.z)
    {
        pos.z = maxBounds.z - radius - 1e-5f;
        if (vel.z > 0) vel.z *= -collisionDamping;
        collided = true;
    }

    // �浹 �߻� �� ������Ʈ�� ������ ����
    if (collided)
    {
        p_i.position = pos;
        p_i.velocity = vel;
    }

	ParticlesOutput[index] = p_i;
}