
cbuffer GlobalConstants : register(b0)
{
	float4x4 view;
	float4x4 proj;
	float4x4 viewProj;
	float4x4 invProj;

	float3 eyeWorld;
	float strengthIBL;

	int choiceEnvMap;
	float envLodBias;
	int mode;
	float depthScale;

	float fogStrength;
	uint depthOnlySRVIndex;
	uint shadowDepthOnlyStartIndex;
	uint resolvedSRVIndex;

	uint fogSRVIndex;
	uint isEnvEnabled;
	float d01;
	float d02;

	float4x4 d03;
	float4x4 d04;
	float4x4 d05;
}

#define PI 3.1415926535

// 3차원 Poly6 커널 함수
float Poly6_3D(float r, float h)
{
	if (r >= h) return 0.0;

	float C = 315.0 / (64.0 * PI * pow(h, 9.0));
	return C * pow((h * h - r * r), 3);
}

float3 LinearToneMapping(float3 color)
{
	float3 invGamma = float3(1, 1, 1) / 2.2;

	color = clamp(1.0 * color, 0., 1.);
	color = pow(color, invGamma);
	return color;
}

struct PSInput
{
	float4 clipPos : SV_POSITION;
	float2 texCoord : TEXCOORD;
	float3 color : COLOR;
	float radius : PSIZE1;
	uint primID : SV_PrimitiveID;
};

float4 main(PSInput input) : SV_TARGET
{
	float2 offset = (float2(0.5, 0.5) - input.texCoord) * 2.0;
	float sqrDst = dot(offset, offset);

	if (sqrDst > 1) discard;

	float3 color = float3(0.7f, 0.7f, 0.8f);

	float ndcZ = input.clipPos.z;

	float z = sqrt(1 - sqrDst);
	float depth = ndcZ - z * input.radius;

	return float4(depth, depth, depth, depth);
}
