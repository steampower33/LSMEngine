struct Particle {
    float2 position;
    float2 velocity;
    float4 color;
    float life;
    float size;
    float padding1;
    float padding2;
};

cbuffer SimParams : register(b0) {
    float deltaTime;
    float2 gravity;
    uint numParticles;
    float2 minBounds;
    float2 maxBounds;
};

StructuredBuffer<Particle> ParticlesInput : register(t0);
RWStructuredBuffer<Particle> ParticlesOutput : register(u0);

[numthreads(64, 1, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint index = dispatchThreadID.x;

    if (index >= numParticles) {
        return;
    }

    Particle p = ParticlesInput[index];

    // 생명 주기 확인 및 파티클 업데이트 로직
    if (p.life <= 0.0f) {
        // 비활성 파티클 처리 또는 리셋
        p.position = float2(float(index / 64), 0.0); // 시작
        p.velocity = float2(((index % 100) / 50.0) - 1.0, 0.0); // 약간 랜덤 좌우 속도
        p.life = 1.0 + (index % 10) / 20.0; // 랜덤 수명 (1.0 ~ 1.5초 가정)
        p.color = float4(0.0, 0.0, 1.0, 1.0); // 색상 초기화
    }
    else {
        // 중력 적용
        p.velocity += gravity * deltaTime;

        // 위치 업데이트
        p.position += p.velocity * deltaTime;

        // 경계 처리
        bool collided = false;
        float2 reflected_vel = p.velocity;

        if (p.position.x < minBounds.x) {}
        else if (p.position.x > maxBounds.x) {}

        if (p.position.y < minBounds.y) {}
        else if (p.position.y > maxBounds.y) {}

        if (collided) { p.velocity = reflected_vel; }

        // 생명 주기 감소
        p.life -= deltaTime; // 1.0이 1초를 의미한다고 가정
    }

    // 쓰기
    ParticlesOutput[index] = p;
}