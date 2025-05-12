
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
	float4 clipPos  : SV_POSITION;   // �������ǵ� ���� �ڳ�
	float2 texCoord : TEXCOORD0;     // ���� UV (0~1)
	float3 viewPos  : TEXCOORD1;     // �� ���������� ���� �߽�
	float  radius : PSIZE1;        // ���� ������
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

	float thicknessContribution = sqrt(1.0 - r2);

	thicknessContribution *= thicknessContributionScale;

	o.thicknessContribution = float4(thicknessContribution, 0.0, 0.0, 0.0);

	return o;
}
