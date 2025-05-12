
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
	float4 thicknessContribution : SV_Target0;
};

PSOutput main(PSInput input)
{
	PSOutput o;

	float3 N;
	N.xy = input.texCoord.xy * 2.0 - 1.0;

	float r2 = dot(N.xy, N.xy);

	if (r2 > 1.0) discard;

	float contribution = exp(-r2 * 4.0) * 1.0;

	o.thicknessContribution = float4(contribution, 0.0, 0.0, 0.0);

	return o;
}
