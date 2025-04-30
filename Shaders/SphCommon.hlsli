
#define PI 3.1415926535

struct Particle {
    float3 position;
    float3 velocity;
    float3 force;
    float density;
    float nearDensity;
    float3 predictedPosition;
    uint isGhost;
    float spawnTime;
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
    float radius;
    float density0;

    float viscosity;
    float gravityCoeff;
    float collisionDamping;
    uint forceKey;

    float currentTime;
    float pressureCoeff;
    float nearPressureCoeff;
};

uint GetCellKeyFromCellID(int3 cellID)
{
    // 큰 소수를 사용하여 좌표를 섞어줌
    const uint p1 = 73856093;
    const uint p2 = 19349663;
    const uint p3 = 83492791;

    int k = cellID.x;
    int l = cellID.y;
    int m = cellID.z;

    uint hashValue = (uint)(k * p1) ^ (uint)(l * p2) ^ (uint)(m * p3);

    return hashValue % cellCnt;

    //uint hashValue =
    //    cellID.x +
    //    cellID.y * gridDimX +
    //    cellID.z * gridDimX * gridDimY;

    //return hashValue;
}

// 간단한 정수 해시 함수 (결과를 [0, 1] 범위의 float로 변환)
float random(uint seed)
{
    seed = seed * 747796405u + 2891336453u;
    uint result = ((seed >> ((seed >> 28u) + 4u)) ^ seed) * 277803737u;
    result = (result >> 22u) ^ result;
    // 결과를 [0, 1] 범위의 float로 정규화
    return float(result) / 4294967295.0f; // 2^32 - 1 로 나눔
}

// 2차원 Poly6 커널 함수
float Poly6_2D(float r, float h)
{
    if (r >= h) return 0.0;

    float C = 4.0 / (PI * pow(h, 8.0));
    return C * pow((h * h - r * r), 3.0);
}

// 2차원 Spiky Gradient 커널 함수
float SpikyGradient_2D(float r, float h)
{
    if (r >= h) return 0.0;

    float C_grad = 30.0 / (PI * pow(h, 5.0));

    return C_grad * pow(h - r, 2.0);
}

// 2차원 Viscosity Laplacian 커널 함수
float ViscosityLaplacian_2D(float r, float h)
{
    if (r >= h) return 0.0;

    float C = 40.0 / (PI * pow(h, 5.0));
    return C * (h - r);
}

// 3차원 Poly6 커널 함수
float Poly6_3D(float r, float h)
{
    if (r >= h) return 0.0;

    float C = 315.0 / (64.0 * PI * pow(h, 9.0));
    return C * pow((h * h - r * r), 3);
}

// 3차원 Spiky Gradient 커널 함수
float SpikyGradient_3D(float r, float h)
{
    if (r >= h) return 0.0;

    float C_grad = 45.0f / (PI * pow(h, 6.0));

    return C_grad * pow(h - r, 2.0);
}

// 3차원 Viscosity Laplacian 커널 함수
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

float SmoothingKernelPoly6(float dst, float radius)
{
    if (dst < radius)
    {
        float scale = 315 / (64 * PI * pow(abs(radius), 9));
        float v = radius * radius - dst * dst;
        return v * v * v * scale;
    }
    return 0;
}

// 3d conversion: done
float SpikyKernelPow3(float dst, float radius)
{
    if (dst < radius)
    {
        float scale = 15 / (PI * pow(radius, 6));
        float v = radius - dst;
        return v * v * v * scale;
    }
    return 0;
}

// 3d conversion: done
//Integrate[(h-r)^2 r^2 Sin[θ], {r, 0, h}, {θ, 0, π}, {φ, 0, 2*π}]
float SpikyKernelPow2(float dst, float radius)
{
    if (dst < radius)
    {
        float scale = 15 / (2 * PI * pow(radius, 5));
        float v = radius - dst;
        return v * v * scale;
    }
    return 0;
}

// 3d conversion: done
float DerivativeSpikyPow3(float dst, float radius)
{
    if (dst <= radius)
    {
        float scale = 45 / (pow(radius, 6) * PI);
        float v = radius - dst;
        return -v * v * scale;
    }
    return 0;
}

// 3d conversion: done
float DerivativeSpikyPow2(float dst, float radius)
{
    if (dst <= radius)
    {
        float scale = 15 / (pow(radius, 5) * PI);
        float v = radius - dst;
        return -v * scale;
    }
    return 0;
}

float DensityKernel(float dst, float radius)
{
    return SpikyKernelPow2(dst, radius);
}

float NearDensityKernel(float dst, float radius)
{
    return SpikyKernelPow3(dst, radius);
}

float DensityDerivative(float dst, float radius)
{
    return DerivativeSpikyPow2(dst, radius);
}

float NearDensityDerivative(float dst, float radius)
{
    return DerivativeSpikyPow3(dst, radius);
}

float PressureFromDensity(float density, float targetDensity, float pressureCoeff)
{
    return (density - targetDensity) * pressureCoeff;
}

float NearPressureFromDensity(float nearDensity, float nearPressureCoeff)
{
    return nearDensity * nearPressureCoeff;
}
