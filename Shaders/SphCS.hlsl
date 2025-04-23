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

	Particle p = ParticlesInput[index];

	/*p.currentAcceleration = p.force / p.density;
	p.velocity = p.velocityHalfStep + p.currentAcceleration * (deltaTime / 2.0f);*/

	p.velocity += p.force / mass * deltaTime;
	p.position += p.velocity * deltaTime;

	if (p.position.x - p.radius <= minBounds.x)
	{
		p.velocity.x *= -collisionDamping;
		p.position.x = minBounds.x + p.radius;
	}
	else if (p.position.x + p.radius >= maxBounds.x)
	{
		p.velocity.x *= -collisionDamping;
		p.position.x = maxBounds.x - p.radius ;
	}

	if (p.position.y - p.radius <= minBounds.y)
	{
		p.velocity.y *= -collisionDamping;
		p.position.y = minBounds.y + p.radius ;
	}
	else if (p.position.y + p.radius  >= maxBounds.y)
	{
		p.velocity.y *= -collisionDamping;
		p.position.y = maxBounds.y - p.radius;
	}

	if (p.position.z - p.radius <= minBounds.z)
	{
		p.velocity.z *= -collisionDamping;
		p.position.z = minBounds.z + p.radius;
	}
	else if (p.position.z + p.radius >= maxBounds.z)
	{
		p.velocity.z *= -collisionDamping;
		p.position.z = maxBounds.z - p.radius;
	}

	//// �ӵ� ���
	//float maxSpeed = 5.0;
	//float speed = length(p.velocity);
	//float speedT = saturate(speed / maxSpeed);

	//// �ӵ��� ���� ���� ��� (�ϴû�, �Ķ���, �Ͼ��)
	//float3 color = lerp(float3(0.403f, 0.537f, 0.749f), float3(0.0, 0.0, 1.0), speedT); // �ϴû� -> �Ķ���

	//// ������ �帣�� �κ��� ����� ���Խ�Ŵ
	//if (speedT > 0.8) {
	//	color = lerp(color, float3(1.0, 1.0, 1.0), 0.2); // �Ͼ�� ����
	//}
	////p.life -= deltaTime;

	//p.color = color;

	//ParticlePosition[index] = p.position;

	// ����
	ParticlesOutput[index] = p;
}