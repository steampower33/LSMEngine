
#define PI 3.1415926535

struct Particle {
    float3 position;
    float radius;
    float3 velocity;
    float life;
    float3 color;
    float density;
    float3 force;
    float pressure;
    float3 predictedPosition;
    float p1;
};

struct ParticleHash
{
    uint particleID; // ���� ��ƼŬ �ε���
    uint hashValue;  // ���� �ؽ� ��
    uint flag;       // �׷� Flag  
};

struct ScanResult
{
    uint groupID;
};

struct CompactCell
{
    uint hashValue;
    uint startIndex;
    uint endIndex;
};

#ifndef GROUP_SIZE_X
    #define GROUP_SIZE_X 512
#endif

cbuffer SimParams : register(b0) {
    float deltaTime;
    uint numParticles;
    float smoothingRadius;
    uint cellCnt;

    float3 minBounds;
    int gridDimX;
    float3 maxBounds;
    int gridDimY;

    int gridDimZ;
    uint maxParticles;
    float mass;
    float pressureCoeff;

    float density0;
    float viscosity;
    float gravity;
    float collisionDamping;
};

// ������ ���� �ؽ� �Լ� (����� [0, 1] ������ float�� ��ȯ)
float random(uint seed)
{
    seed = seed * 747796405u + 2891336453u;
    uint result = ((seed >> ((seed >> 28u) + 4u)) ^ seed) * 277803737u;
    result = (result >> 22u) ^ result;
    // ����� [0, 1] ������ float�� ����ȭ
    return float(result) / 4294967295.0f; // 2^32 - 1 �� ����
}

// Poly6
float SmoothingKernel(float dst, float radius)
{
    if (dst >= radius) return 0;

    float C = 4.0 / (PI * pow(radius, 8)); // 2���� Poly6 Ŀ�� ����ȭ ���
    return C * pow((radius * radius - dst * dst), 3); // 2���� Poly6 Ŀ�� �Լ�
}

float SmoothingKerenlPressure(float dst, float radius)
{
    if (dst >= radius) return 0;

    float C = 10.0 / (PI * pow(radius, 5)); // 2���� Spiky Ŀ�� ����ȭ ���
    return C * pow((radius - dst), 3); // 2���� Spiky Ŀ�� �Լ�
}

float SmoothingKernelLaplacian(float dst, float radius)
{
    if (dst >= radius) return 0;

    float C = 40.0 / (7.0 * PI * radius * radius); // 2���� ������ Ŀ�� ����ȭ ���
    return C * (radius - dst); // 2���� ������ Ŀ�� �Լ�
}