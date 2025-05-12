
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

struct GSInput
{
    float4 clipPos    : SV_POSITION;  // 투영된 클립 좌표
    float3 viewPos  : TEXCOORD0;    // 뷰 공간 좌표
    float  radius : PSIZE0;
};

struct PSInput
{
    float4 clipPos  : SV_POSITION;   // 프로젝션된 쿼드 코너
    float2 texCoord : TEXCOORD0;     // 쿼드 UV (0~1)
    float3 viewPos  : TEXCOORD1;     // 뷰 공간에서의 입자 중심
    float  radius : PSIZE1;        // 입자 반지름
    uint primID : SV_PrimitiveID;
};

[maxvertexcount(4)]
void main(point GSInput input[1], uint primID : SV_PrimitiveID,
	inout TriangleStream<PSInput> outputStream)
{

    float hw = input[0].radius; // halfWidth
    float3 viewCenter = input[0].viewPos;

    float3 up = float3(0, hw, 0);
    float3 right = float3(hw, 0, 0);

    PSInput output;

    output.primID = primID;
    output.radius = input[0].radius;
    output.viewPos = viewCenter;

    // Bottom-Left (-right - up)
    output.clipPos = mul(float4(viewCenter - right - up, 1.0), proj);
    output.texCoord = float2(0.0, 0.0);
    outputStream.Append(output);

    // Top-Left (-right + up)
    output.clipPos = mul(float4(viewCenter - right + up, 1.0), proj);
    output.texCoord = float2(0.0, 1.0);
    outputStream.Append(output);

    // Bottom-Right (+right - up)
    output.clipPos = mul(float4(viewCenter + right - up, 1.0), proj);
    output.texCoord = float2(1.0, 0.0);
    outputStream.Append(output);

    // Top-Right (+right + up)
    output.clipPos = mul(float4(viewCenter + right + up, 1.0), proj);
    output.texCoord = float2(1.0, 1.0);
    outputStream.Append(output);

    outputStream.RestartStrip();
}