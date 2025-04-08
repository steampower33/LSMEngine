#include "SphCommon.hlsli"

StructuredBuffer<ParticleHash> ParticlesHashes : register(t0);

RWStructuredBuffer<Particle> Particles : register(u0);

// 간단한 정수 해시 함수 (결과를 [0, 1] 범위의 float로 변환)
float random(uint seed)
{
    // 간단한 해싱 알고리즘
    seed = seed * 747796405u + 2891336453u;
    uint result = ((seed >> ((seed >> 28u) + 4u)) ^ seed) * 277803737u;
    result = (result >> 22u) ^ result;
    // 결과를 [0, 1] 범위의 float로 정규화
    return float(result) / 4294967295.0f; // 2^32 - 1 로 나눔
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

    // 생명 주기 확인 및 파티클 업데이트 로직
    p.velocity += (p.force / mass) * deltaTime;
    p.position += p.velocity * deltaTime;

    p.velocity += float3(0.0, -9.8, 0.0) * deltaTime;

    // 경계 처리
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

    // 생명 주기 감소
    //p.life -= deltaTime; // 1.0이 1초를 의미한다고 가정

    //if ((hashInfo.hashValue % 2) == 0) {
    //    p.color = float3(1.0, 0.0, 0.0); // Red if even
    //}
    //else {
    //    p.color = float3(0.0, 1.0, 0.0); // Green if odd
    //}

    // 쓰기
    Particles[index] = p;
}