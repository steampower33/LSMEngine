
cbuffer RenderParams : register(b0)
{
	float4x4 view;
	float4x4 proj;

	float thicknessContributionScale;
	float p1;
	float p2;
	float p3;

	float4 p4;
	float4 p5;
	float4 p6;

	float4x4 p7;
};

struct PSInput
{
	float4 clipPos  : SV_POSITION;   // 프로젝션된 쿼드 코너
	float2 texCoord : TEXCOORD0;     // 쿼드 UV (0~1)
	float3 viewPos  : TEXCOORD1;     // 뷰 공간에서의 입자 중심
	float  radius : PSIZE1;        // 입자 반지름
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
	N.xy = input.texCoord.xy * 2.0 - 1.0;
	float r2 = dot(N.xy, N.xy);
	if (r2 > 1.0) discard;

	N.z = -sqrt(1.0 - r2);

	float3 pixelPos = input.viewPos + N * input.radius;
	float4 clipSpacePos = mul(float4(pixelPos, 1.0), proj);
	o.depth = clipSpacePos.z / clipSpacePos.w;

	float depth = pixelPos.z;
	o.linearDepth = depth;

	float dNorm = (depth - 0.1) / (100.0 - 0.1);

	N = N * 0.5 + 0.5;
	o.color = float4(N, 1.0);

	return o;
}
