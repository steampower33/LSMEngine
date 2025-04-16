
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
    float3 velocityHalfStep;
    float p1;
    float3 currentAcceleration;
    float p2;
    float3 predictedPosition;
    float p3;
};

struct ParticleHash
{
    uint particleID;
    uint cellKey;
    uint flag;
};

struct CellStart
{
    uint startIndex;
};

struct ScanResult
{
    uint groupID;
};

struct CompactCell
{
    uint cellKey;
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

    uint forceKey;
};

uint GetCellKeyFromCellID(int3 cellID)
{
    // ū �Ҽ��� ����Ͽ� ��ǥ�� ������
    uint p1 = 73856093;
    uint p2 = 19349663;

    int k = cellID.x;
    int l = cellID.y;

    uint hashValue = (uint)(k * p1) ^ (uint)(l * p2);

    return hashValue % cellCnt;
}

// ������ ���� �ؽ� �Լ� (����� [0, 1] ������ float�� ��ȯ)
float random(uint seed)
{
    seed = seed * 747796405u + 2891336453u;
    uint result = ((seed >> ((seed >> 28u) + 4u)) ^ seed) * 277803737u;
    result = (result >> 22u) ^ result;
    // ����� [0, 1] ������ float�� ����ȭ
    return float(result) / 4294967295.0f; // 2^32 - 1 �� ����
}

// 2���� Poly6 Ŀ�� �Լ�
float Poly6_2D(float r, float h)
{
    if (r >= h) return 0.0;

    float C = 4.0 / (PI * pow(h, 8));
    return C * pow((h * h - r * r), 3);
}

// 2���� Spiky Gradient Ŀ�� �Լ�
float SpikyGradient_2D(float r, float h)
{
    if (r >= h) return 0.0;

    float h_pow5 = pow(h, 5);

    float C_grad = 30.0f / (PI * h_pow5);

    return C_grad * pow(h - r, 2);
}

// 2���� Viscosity Laplacian Ŀ�� �Լ�
float ViscosityLaplacian_2D(float r, float h)
{
    if (r >= h) return 0.0;

    float C = 40.0 / (PI * pow(h, 5.0));
    return C * (h - r);
}