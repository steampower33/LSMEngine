
#define PI 3.1415926535
#define COR 0.8

struct Particle {
    float3 position;
    float radius;
    float3 velocity;
    float life;
    float3 color;
    float density;
    float3 force;
    float pressure;
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
    float cellSize;
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
};

float CubicSpline(float q)
{
    float coeff = 3.0 / (2.0 * PI);

    if (q < 1.0)
        return coeff * (2.0 / 3.0 - q * q + 0.5 * q * q * q);
    else if (q < 2.0)
        return coeff * pow(2.0 - q, 3.0) / 6.0;
    else // q >= 2.0f
        return 0.0;
}

float CubicSplineGrad(float q)
{
    float coeff = 3.0 / (2.0 * PI);

    if (q < 1.0)
        return coeff * (-2.0 * q + 1.5 * q * q);
    else if (q < 2.0)
        return coeff * -0.5 * (2.0 - q) * (2.0 - q);
    else // q >= 2.0f
        return 0.0;
}
