struct PSInput
{
	float4 position : SV_Position;
};

float4 main(PSInput input) : SV_TARGET
{
	return float4(1.0, 1.0, 1.0, 1.0);
}