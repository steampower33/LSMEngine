struct PSInput
{
	float4 position : SV_Position;
};

float4 main(PSInput input) : SV_TARGET
{
	return input.position;
}