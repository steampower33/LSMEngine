
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
	float radius : PSIZE1;
	uint primID : SV_PrimitiveID;
};

struct PS_OUTPUT
{
	float4 color : SV_Target0;
	float customDepth : SV_Target1;
};

PS_OUTPUT main(PSInput input)
{
	float2 offset = (float2(0.5, 0.5) - input.texCoord) * 2.0;
	float sqrDst = dot(offset, offset);

	if (sqrDst > 1) discard;

	float3 normal = normalize(float3(offset.x, offset.y, sqrt(1 - sqrDst)));
	
	float3 lightDir = normalize(float3(0, 0, 1));
	float  NdotL = saturate(dot(normal, lightDir));
	float3 color = float3(1.0, 1.0, 1.0) * NdotL;

	//return float4(LinearToneMapping(color), 1);
	//return float4(color, 1);

	PS_OUTPUT output;

	output.color = float4(color, 1.0);
	output.customDepth = 1.0 - input.clipPos.z / input.clipPos.w;
	return output;
}
