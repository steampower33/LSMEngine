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

// ������ ���� �ؽ� �Լ� (����� [0, 1] ������ float�� ��ȯ)
// �Է� �õ�(seed)�� ������� ���� ���� �����մϴ�.
float random(uint seed)
{
    // ������ �ؽ� �˰���
    seed = seed * 747796405u + 2891336453u;
    uint result = ((seed >> ((seed >> 28u) + 4u)) ^ seed) * 277803737u;
    result = (result >> 22u) ^ result;
    // ����� [0, 1] ������ float�� ����ȭ
    return float(result) / 4294967295.0f; // 2^32 - 1 �� ����
}

// 2���� uint�� �õ�� �޴� ����
float random2(uint2 seed)
{
    // �õ带 �����Ͽ� �ϳ��� uint�� ����ϴ�.
    seed.x += deltaTime; // �ð� ��Ҹ� �߰��Ͽ� �� ������ �ٸ��� ����
    uint combinedSeed = seed.x ^ (seed.y * 747796405u);
    return random(combinedSeed);
}

// float3 ���� ���� ���� (��: -1 ~ +1 ����)
float3 randomDirection(uint2 seed)
{
    float x = random2(seed) * 2.0f - 1.0f; // [-1, 1]
    float y = random2(seed + uint2(1, 3)) * 2.0f - 1.0f; // �õ带 �ణ �����Ͽ� �ٸ� �� ����
    //float z = random2(seed + uint2(5, 7)) * 2.0f - 1.0f;
    float z = 0.0;
    return normalize(float3(x, y, z)); // ����ȭ�� ���� ����
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

    //// ���� �ֱ� Ȯ�� �� ��ƼŬ ������Ʈ ����
    //if (p.life <= 0.0f) {

    //    // ������ �õ� ���� (��ƼŬ �ε����� ������ ī��Ʈ/�ð� ����)
    //    // deltaTime�� ���ϰų� XOR �ϴ� �� �پ��ϰ� ���� ����
    //    uint2 seed = uint2(index, deltaTime); // �Ǵ� uint2(index, uint(totalTime * 1000.0f)) ��

    //    // ���� ��ġ ����
    //    float spawnRadius = 0.1f;
    //    p.position = float3(0.0, 0.0, 0.0) + randomDirection(seed + uint2(10, 11)) * random2(seed + uint2(12, 13)) * spawnRadius;

    //    // ���� �ӵ� ����
    //    float initialSpeedMin = 1.0f;
    //    float initialSpeedMax = 3.0f;
    //    float initialSpeed = lerp(initialSpeedMin, initialSpeedMax, random2(seed + uint2(1, 2)));
    //    // ��: ���� �ݱ� �������� �����ϰ� �߻�
    //    float angle = random2(seed + uint2(3, 4)) * 2.0f * PI;
    //    float elevation = random2(seed + uint2(5, 6)) * PI * 0.5f; // 0 ~ 90��
    //    //float3 randomDir = float3(cos(angle) * cos(elevation), sin(elevation), sin(angle) * cos(elevation));
    //    float3 randomDir = float3(cos(angle) * cos(elevation), sin(elevation), 0.0f);
    //    p.velocity = normalize(randomDir + float3(0, 0.5, 0)) * initialSpeed; // �ణ �������� ���� + ����ȭ �� �ӵ� ���ϱ�

    //    // ���� ���� �ֱ� ����
    //    float minLife = 0.5f;
    //    float maxLife = 1.5f;
    //    p.life = lerp(minLife, maxLife, random2(seed + uint2(9, 10)));
    //}
    //else 
    {
        // �߷� ����
        p.velocity += float3(gravity, 0.0) * deltaTime;

        // ��ġ ������Ʈ
        p.position += p.velocity * deltaTime;

        // ��� ó��
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

        // ���� �ֱ� ����
        //p.life -= deltaTime; // 1.0�� 1�ʸ� �ǹ��Ѵٰ� ����
    }

    if ((hashInfo.hashValue % 2) == 0) {
        p.color = float3(1.0, 0.0, 0.0); // Red if even
    }
    else {
        p.color = float3(0.0, 1.0, 0.0); // Green if odd
    }

    // ����
    ParticlesOutput[index] = p;
}