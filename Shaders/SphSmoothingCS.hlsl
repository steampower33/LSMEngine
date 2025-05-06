
Texture2D<float> DepthTexture : register(t1);

RWTexture2D<float4> SmoothingDepthTexture : register(u0);

[numthreads(16, 16, 1)]
void main(uint tid : SV_GroupThreadID,
	uint3 gtid : SV_DispatchThreadID,
	uint groupIdx : SV_GroupID)
{
	uint width, height;
	SmoothingDepthTexture.GetDimensions(width, height);

	uint2 pixelCoords = gtid.xy;

	if (pixelCoords.x >= width || pixelCoords.y >= height)
	{
		return;
	}
	float4 depth = DepthTexture.Load(int3(pixelCoords, 0));

	SmoothingDepthTexture[pixelCoords] = depth;
}