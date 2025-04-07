#include "SphCommon.hlsli"

cbuffer SimParams : register(b0) {
    float deltaTime;
    float2 gravity;
    uint numParticles;
    float3 minBounds;
    float gridX;
    float3 maxBounds;
    float gridY;

    float gridZ;
    float p1;
    float p2;
    float p3;
};

#define PI 3.1415926535
#define COR 1.0

StructuredBuffer<Particle> ParticlesInput : register(t0);
RWStructuredBuffer<Particle> ParticlesOutput : register(u0);

StructuredBuffer<ParticleHash> ParticlesHashes : register(t1);

// 간단한 정수 해시 함수 (결과를 [0, 1] 범위의 float로 변환)
// 입력 시드(seed)를 기반으로 랜덤 값을 생성합니다.
float random(uint seed)
{
    // 간단한 해싱 알고리즘
    seed = seed * 747796405u + 2891336453u;
    uint result = ((seed >> ((seed >> 28u) + 4u)) ^ seed) * 277803737u;
    result = (result >> 22u) ^ result;
    // 결과를 [0, 1] 범위의 float로 정규화
    return float(result) / 4294967295.0f; // 2^32 - 1 로 나눔
}

// 2개의 uint를 시드로 받는 버전
float random2(uint2 seed)
{
    // 시드를 결합하여 하나의 uint로 만듭니다.
    seed.x += deltaTime; // 시간 요소를 추가하여 매 프레임 다르게 만듬
    uint combinedSeed = seed.x ^ (seed.y * 747796405u);
    return random(combinedSeed);
}

// float3 랜덤 벡터 생성 (예: -1 ~ +1 범위)
float3 randomDirection(uint2 seed)
{
    float x = random2(seed) * 2.0f - 1.0f; // [-1, 1]
    float y = random2(seed + uint2(1, 3)) * 2.0f - 1.0f; // 시드를 약간 변경하여 다른 값 생성
    //float z = random2(seed + uint2(5, 7)) * 2.0f - 1.0f;
    float z = 0.0;
    return normalize(float3(x, y, z)); // 정규화된 방향 벡터
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

    Particle p = ParticlesInput[index];
    float halfSize = p.size * 0.5;

    ParticleHash hashInfo = ParticlesHashes[index];

    //// 생명 주기 확인 및 파티클 업데이트 로직
    //if (p.life <= 0.0f) {

    //    // 고유한 시드 생성 (파티클 인덱스와 프레임 카운트/시간 조합)
    //    // deltaTime를 더하거나 XOR 하는 등 다양하게 조합 가능
    //    uint2 seed = uint2(index, deltaTime); // 또는 uint2(index, uint(totalTime * 1000.0f)) 등

    //    // 랜덤 위치 설정
    //    float spawnRadius = 0.1f;
    //    p.position = float3(0.0, 0.0, 0.0) + randomDirection(seed + uint2(10, 11)) * random2(seed + uint2(12, 13)) * spawnRadius;

    //    // 랜덤 속도 설정
    //    float initialSpeedMin = 1.0f;
    //    float initialSpeedMax = 3.0f;
    //    float initialSpeed = lerp(initialSpeedMin, initialSpeedMax, random2(seed + uint2(1, 2)));
    //    // 예: 위쪽 반구 방향으로 랜덤하게 발사
    //    float angle = random2(seed + uint2(3, 4)) * 2.0f * PI;
    //    float elevation = random2(seed + uint2(5, 6)) * PI * 0.5f; // 0 ~ 90도
    //    //float3 randomDir = float3(cos(angle) * cos(elevation), sin(elevation), sin(angle) * cos(elevation));
    //    float3 randomDir = float3(cos(angle) * cos(elevation), sin(elevation), 0.0f);
    //    p.velocity = normalize(randomDir + float3(0, 0.5, 0)) * initialSpeed; // 약간 위쪽으로 편향 + 정규화 후 속도 곱하기

    //    // 랜덤 생명 주기 설정
    //    float minLife = 0.5f;
    //    float maxLife = 1.5f;
    //    p.life = lerp(minLife, maxLife, random2(seed + uint2(9, 10)));
    //}
    //else 
    {
        // 중력 적용
        p.velocity += float3(gravity, 0.0) * deltaTime;

        // 위치 업데이트
        p.position += p.velocity * deltaTime;

        // 경계 처리
        bool collided = false;
        float3 reflected_vel = p.velocity;

        if (p.position.x - halfSize < minBounds.x && p.velocity.x < 0.0)
        {
            p.velocity.x *= -COR;
            p.position.x = minBounds.x + halfSize;
        }
        else if (p.position.x + halfSize > maxBounds.x && p.velocity.x > 0.0)
        {
            p.velocity.x *= -COR;
            p.position.x = maxBounds.x - halfSize;
        }

        if (p.position.y - halfSize < minBounds.y && p.velocity.y < 0.0)
        {
            p.velocity.y *= -COR;
            p.position.y = minBounds.y + halfSize;
        }
        else if (p.position.y + halfSize > maxBounds.y && p.velocity.y > 0.0)
        { 
            p.velocity.y *= -COR;
            p.position.y = maxBounds.y - halfSize;
        }

        if (collided) { p.velocity = reflected_vel; }

        // 생명 주기 감소
        //p.life -= deltaTime; // 1.0이 1초를 의미한다고 가정
    }

    if ((hashInfo.hashValue % 2) == 0) {
        p.color = float3(1.0, 0.0, 0.0); // Red if even
    }
    else {
        p.color = float3(0.0, 1.0, 0.0); // Green if odd
    }

    // 쓰기
    ParticlesOutput[index] = p;
}