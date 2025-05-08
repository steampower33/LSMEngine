
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

	float2 offset = input.texCoord * 2 - 1;
	if (dot(offset, offset) > 1) discard;

	float3 N = float3(offset, -sqrt(1 - dot(offset, offset)));
	float3 viewPos = input.viewPos + N * input.radius;

	float4 clipPos = mul(float4(viewPos, 1.0), proj);

	// GPU¿ë Depth
	float depth = clipPos.z / clipPos.w;

	float linearZ = viewPos.z;
	float dNorm = saturate((linearZ - 0.01) / (20.0 - 0.01));
	o.linearDepth = dNorm;
	o.color = float4(dNorm, dNorm, dNorm, 1.0);
	o.depth = depth;
	return o;
}
