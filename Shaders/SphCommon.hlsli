
#define PI 3.1415926535

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
	float currentTime;
	float3 maxBounds;
	float endTime;

	int gridDimX;
	int gridDimY;
	int gridDimZ;
	uint forceKey;

	float density0;
	float pressureCoeff;
	float nearPressureCoeff;
	float viscosity;

	float mass;
	float radius;
	float boundaryStiffness;
	float boundaryDamping;

	float gravityCoeff;
	float duration;
	float startTime;
	uint reset;

	uint nX;
	uint nY;
	uint nZ;
	float dp;

	float4x4 p3;
	float4x4 p4;
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

// 3차원 Poly6 커널 함수
float Poly6Kernel(float r, float h)
{
	if (r >= h) return 0.0;

	float C = 315.0 / (64.0 * PI * pow(h, 9.0));
	return C * pow((h * h - r * r), 3);
}

// 3차원 Spiky Gradient 커널 함수
float SpikyGradient(float r, float h)
{
	if (r >= h) return 0.0;

	float C_grad = 45.0f / (PI * pow(h, 6.0));

	return C_grad * pow(h - r, 2.0);
}

// 3차원 Viscosity Laplacian 커널 함수
float ViscosityLaplacian(float r, float h)
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

float SpikyKernelPow3(float r, float h)
{
	if (r >= h) return 0.0;

	float scale = 15 / (PI * pow(h, 6));
	float v = h - r;
	return v * v * v * scale;
}

float SpikyKernelPow2(float r, float h)
{
	if (r >= h) return 0.0;

	float scale = 15 / (2 * PI * pow(h, 5));
	float v = h - r;
	return v * v * scale;
}

float DerivativeSpikyPow3(float r, float h)
{
	if (r >= h) return 0.0;

	float scale = 45 / (pow(h, 6) * PI);
	float v = h - r;
	return -v * v * scale;
}

float DerivativeSpikyPow2(float r, float h)
{
	if (r >= h) return 0.0;
	float scale = 15 / (pow(h, 5) * PI);
	float v = h - r;
	return -v * scale;
}

float DensityKernel(float r, float h)
{
	return SpikyKernelPow2(r, h);
}

float NearDensityKernel(float r, float h)
{
	return SpikyKernelPow3(r, h);
}

float DensityDerivative(float r, float h)
{
	return DerivativeSpikyPow2(r, h);
}

float NearDensityDerivative(float r, float h)
{
	return DerivativeSpikyPow3(r, h);
}

float PressureFromDensity(float density, float targetDensity, float pressureCoeff)
{
	return (density - targetDensity) * pressureCoeff;
}

float NearPressureFromDensity(float nearDensity, float nearPressureCoeff)
{
	return nearDensity * nearPressureCoeff;
}
