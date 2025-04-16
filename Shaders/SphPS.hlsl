
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
	float2 texCoord : TEXCOORD;
	float3 color : COLOR;
	uint primID : SV_PrimitiveID;
};

float4 main(PSInput input) : SV_TARGET
{
	float radius = 0.5;
	float dist = length(float2(0.5, 0.5) - input.texCoord);

	if (dist > radius)
		discard;

	float q = dist / radius;
	float scale = 1.0 - q;
	return float4(LinearToneMapping(input.color.rgb * scale), 1.0);
	//return float4(input.color.rgb * scale, 1.0);
}
