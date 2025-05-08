
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
	float3 viewCenter : TEXCOORD1;
	float radius : PSIZE1;
	uint primID : SV_PrimitiveID;
};

struct PSOutput
{
	float4 color : SV_Target0;
	//float customDepth : SV_Target1;
	float depth : SV_Depth;
};

PSOutput main(PSInput input)
{
	PSOutput o;

	float2 ofs = input.texCoord * 2 - 1;
	if (dot(ofs, ofs) > 1) discard;

	float3 N = float3(ofs, -sqrt(1 - dot(ofs, ofs)));
	float3 viewPos = input.viewCenter + N * input.radius;

	// 3) Z (원근 방향 거리) → 정규화
	float linearZ = viewPos.z;
	float dNorm = saturate((linearZ - 0.1) / (1000.0 - 0.1));

	// 4) 깊이 & 컬러
	o.depth = dNorm;                  // SV_Depth
	o.color = float4(dNorm, dNorm, dNorm, 1);
	return o;

}
