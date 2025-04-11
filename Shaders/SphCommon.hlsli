
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
    uint particleID; // 원래 파티클 인덱스
    uint hashValue;  // 계산된 해시 값
    uint flag;       // 그룹 Flag  
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

// 간단한 정수 해시 함수 (결과를 [0, 1] 범위의 float로 변환)
float random(uint seed)
{
    seed = seed * 747796405u + 2891336453u;
    uint result = ((seed >> ((seed >> 28u) + 4u)) ^ seed) * 277803737u;
    result = (result >> 22u) ^ result;
    // 결과를 [0, 1] 범위의 float로 정규화
    return float(result) / 4294967295.0f; // 2^32 - 1 로 나눔
}

// Poly6
float SmoothingKernel(float dst, float radius)
{
    if (dst >= radius) return 0;

    float C = 4.0 / (PI * pow(radius, 8)); // 2차원 Poly6 커널 정규화 상수
    return C * pow((radius * radius - dst * dst), 3); // 2차원 Poly6 커널 함수
}

float SmoothingKerenlPressure(float dst, float radius)
{
    if (dst >= radius) return 0;

    float C = 10.0 / (PI * pow(radius, 5)); // 2차원 Spiky 커널 정규화 상수
    return C * pow((radius - dst), 3); // 2차원 Spiky 커널 함수
}

float SmoothingKernelLaplacian(float dst, float radius)
{
    if (dst >= radius) return 0;

    float C = 40.0 / (7.0 * PI * radius * radius); // 2차원 점성항 커널 정규화 상수
    return C * (radius - dst); // 2차원 점성항 커널 함수
}