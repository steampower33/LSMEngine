
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

struct GSInput
{
    float4 viewPos : SV_POSITION;
    float3 color : COLOR;
    float life : PSIZE0;
    float radius : PSIZE1;
};

struct PSInput
{
	float4 clipPos : SV_POSITION;
	float2 texCoord : TEXCOORD;
	float3 color : COLOR;
	uint primID : SV_PrimitiveID;
};

[maxvertexcount(4)]
void main(point GSInput input[1], uint primID : SV_PrimitiveID,
	inout TriangleStream<PSInput> outputStream)
{

    float hw = input[0].radius; // halfWidth
    float3 viewCenter = input[0].viewPos.xyz;

    float3 up = float3(0, hw, 0);
    float3 right = float3(hw, 0, 0);

    PSInput output;
    output.color = input[0].color;
    output.primID = primID;

    // 뷰 공간에서 꼭짓점 계산 후, 프로젝션 변환하여 클립 공간 좌표 얻기
    float3 cornerViewPos;

    // Bottom-Left (-right - up)
    cornerViewPos = viewCenter - right - up;
    output.clipPos = mul(float4(cornerViewPos, 1.0f), proj); // 프로젝션 적용
    output.texCoord = float2(0.0, 1.0);
    outputStream.Append(output);

    // Top-Left (-right + up)
    cornerViewPos = viewCenter - right + up;
    output.clipPos = mul(float4(cornerViewPos, 1.0f), proj); // 프로젝션 적용
    output.texCoord = float2(0.0, 0.0);
    outputStream.Append(output);

    // Bottom-Right (+right - up)
    cornerViewPos = viewCenter + right - up;
    output.clipPos = mul(float4(cornerViewPos, 1.0f), proj); // 프로젝션 적용
    output.texCoord = float2(1.0, 1.0);
    outputStream.Append(output);

    // Top-Right (+right + up)
    cornerViewPos = viewCenter + right + up;
    output.clipPos = mul(float4(cornerViewPos, 1.0f), proj); // 프로젝션 적용
    output.texCoord = float2(1.0, 0.0);
    outputStream.Append(output);

    outputStream.RestartStrip();
}