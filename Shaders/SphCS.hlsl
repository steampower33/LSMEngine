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

    // ���� �ֱ� Ȯ�� �� ��ƼŬ ������Ʈ ����
    if (p.life <= 0.0f) {
        // ��Ȱ�� ��ƼŬ ó�� �Ǵ� ����
        p.position = float2(float(index / 64), 0.0); // ����
        p.velocity = float2(((index % 100) / 50.0) - 1.0, 0.0); // �ణ ���� �¿� �ӵ�
        p.life = 1.0 + (index % 10) / 20.0; // ���� ���� (1.0 ~ 1.5�� ����)
        p.color = float4(0.0, 0.0, 1.0, 1.0); // ���� �ʱ�ȭ
    }
    else {
        // �߷� ����
        p.velocity += gravity * deltaTime;

        // ��ġ ������Ʈ
        p.position += p.velocity * deltaTime;

        // ��� ó��
        bool collided = false;
        float2 reflected_vel = p.velocity;

        if (p.position.x < minBounds.x) {}
        else if (p.position.x > maxBounds.x) {}

        if (p.position.y < minBounds.y) {}
        else if (p.position.y > maxBounds.y) {}

        if (collided) { p.velocity = reflected_vel; }

        // ���� �ֱ� ����
        p.life -= deltaTime; // 1.0�� 1�ʸ� �ǹ��Ѵٰ� ����
    }

    // ����
    ParticlesOutput[index] = p;
}