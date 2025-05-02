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

	p_i.position += p_i.velocity * deltaTime;

    float stiffness = 1000.0f; // 반발력 계수, 조절 가능
    float3 pos = p_i.position;
    float3 vel = p_i.velocity;

    // X축 경계
    if (pos.x - radius < minBounds.x) {
        float penetration = minBounds.x + radius - pos.x;
        pos.x += penetration;
        vel.x = max(vel.x, 0.0f); // 벽 안으로 들어가려는 속도 제거
        vel.x *= collisionDamping; // 감쇠 적용
    }
    else if (pos.x + radius > maxBounds.x) {
        float penetration = pos.x + radius - maxBounds.x;
        pos.x -= penetration;
        vel.x = min(vel.x, 0.0f);
        vel.x *= collisionDamping;
    }

    // Y축 경계
    if (pos.y - radius < minBounds.y) {
        float penetration = minBounds.y + radius - pos.y;
        pos.y += penetration;
        vel.y = max(vel.y, 0.0f);
        vel.y *= collisionDamping;
    }
    else if (pos.y + radius > maxBounds.y) {
        float penetration = pos.y + radius - maxBounds.y;
        pos.y -= penetration;
        vel.y = min(vel.y, 0.0f);
        vel.y *= collisionDamping;
    }

    // Z축 경계
    if (pos.z - radius < minBounds.z) {
        float penetration = minBounds.z + radius - pos.z;
        pos.z += penetration;
        vel.z = max(vel.z, 0.0f);
        vel.z *= collisionDamping;
    }
    else if (pos.z + radius > maxBounds.z) {
        float penetration = pos.z + radius - maxBounds.z;
        pos.z -= penetration;
        vel.z = min(vel.z, 0.0f);
        vel.z *= collisionDamping;
    }

    p_i.position = pos;
    p_i.velocity = vel;

	ParticlesOutput[index] = p_i;
}