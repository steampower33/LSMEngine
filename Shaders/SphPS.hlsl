
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
	float2 texCoord : TEXCOORD0;
	float3 viewPos : TEXCOORD1;
	float radius : PSIZE1;
	uint primID : SV_PrimitiveID;
};

struct PSOutput
{
	float4 color : SV_Target0;
	float linearDepth : SV_Target1;
	float depth : SV_Depth;
};

PSOutput main(PSInput input)
{
	PSOutput o;

	float3 N;
	N.xy = input.texCoord * 2.0 - 1.0;
	float r2 = dot(N.xy, N.xy);

	if (r2 > 1.0) discard;

	N.z = sqrt(1.0 - r2);

	float3 pixelPos = input.viewPos + N * input.radius;
	float4 clipSpacePos = mul(float4(pixelPos, 1.0), proj);
	float depth = pixelPos.z;

	float dNorm = (depth - 0.1) / (20.0f - 0.1);

	o.color = float4(dNorm, dNorm, dNorm, 1.0);
	o.depth = clipSpacePos.z / clipSpacePos.w;

	o.linearDepth = depth;

	return o;
}
