
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
    float3 currentAcceleration;
    float p;
};

struct ParticleHash
{
    uint particleID;
    int cellIndex;
    uint flag;
};

struct CompactCell
{
    int cellIndex;
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
    float mass;
    float pressureCoeff;
    float density0;

    float viscosity;
    float gravityCoeff;
    float collisionDamping;
    uint forceKey;
};

uint GetCellKeyFromCellID(int3 cellID)
{
    //// ū �Ҽ��� ����Ͽ� ��ǥ�� ������
    //const uint p1 = 73856093;
    //const uint p2 = 19349663;
    //const uint p3 = 83492791;

    //int k = cellID.x;
    //int l = cellID.y;
    //int m = cellID.z;

    //uint hashValue = (uint)(k * p1) ^ (uint)(l * p2) ^ (uint)(m * p3);

    //return hashValue % cellCnt;

    uint hashValue =
        cellID.x +
        cellID.y * gridDimX +
        cellID.z * gridDimX * gridDimY;

    return hashValue;
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

    float C = 4.0 / (PI * pow(h, 8.0));
    return C * pow((h * h - r * r), 3.0);
}

// 2���� Spiky Gradient Ŀ�� �Լ�
float SpikyGradient_2D(float r, float h)
{
    if (r >= h) return 0.0;

    float C_grad = 30.0 / (PI * pow(h, 5.0));

    return C_grad * pow(h - r, 2.0);
}

// 2���� Viscosity Laplacian Ŀ�� �Լ�
float ViscosityLaplacian_2D(float r, float h)
{
    if (r >= h) return 0.0;

    float C = 40.0 / (PI * pow(h, 5.0));
    return C * (h - r);
}

// 3���� Poly6 Ŀ�� �Լ�
float Poly6_3D(float r, float h)
{
    if (r >= h) return 0.0;

    float C = 315.0 / (64.0 * PI * pow(h, 9.0));
    return C * pow((h * h - r * r), 3);
}

// 3���� Spiky Gradient Ŀ�� �Լ�
float SpikyGradient_3D(float r, float h)
{
    if (r >= h) return 0.0;

    float C_grad = 45.0f / (PI * pow(h, 6.0));

    return C_grad * pow(h - r, 2.0);
}

// 3���� Viscosity Laplacian Ŀ�� �Լ�
float ViscosityLaplacian_3D(float r, float h)
{
    if (r >= h) return 0.0;

    float C = 45.0 / (PI * pow(h, 6.0));
    return C * (h - r);
}

static const int3 offsets3D[27] =
{
    int3(-1, -1, -1),
    int3(-1, -1, 0),
    int3(-1, -1, 1),
    int3(-1, 0, -1),
    int3(-1, 0, 0),
    int3(-1, 0, 1),
    int3(-1, 1, -1),
    int3(-1, 1, 0),
    int3(-1, 1, 1),
    int3(0, -1, -1),
    int3(0, -1, 0),
    int3(0, -1, 1),
    int3(0, 0, -1),
    int3(0, 0, 0),
    int3(0, 0, 1),
    int3(0, 1, -1),
    int3(0, 1, 0),
    int3(0, 1, 1),
    int3(1, -1, -1),
    int3(1, -1, 0),
    int3(1, -1, 1),
    int3(1, 0, -1),
    int3(1, 0, 0),
    int3(1, 0, 1),
    int3(1, 1, -1),
    int3(1, 1, 0),
    int3(1, 1, 1)
};