
Texture2D<float> DepthTexture : register(t1);

RWTexture2D<float> SmoothingDepthTexture : register(u0);

cbuffer RenderParams : register(b0) {
    int filterRadius;
    float sigmaSpatial;
    int sigmaDepth;
};

[numthreads(16, 16, 1)]
void main(uint tid : SV_GroupThreadID,
	uint3 gtid : SV_DispatchThreadID,
	uint groupIdx : SV_GroupID)
{
	uint width, height;
	SmoothingDepthTexture.GetDimensions(width, height);

	uint2 centerCoord = gtid.xy;

	if (centerCoord.x >= width || centerCoord.y >= height)
		return;

	float centerDepth = DepthTexture.Load(int3(centerCoord, 0));

	float totalWeightedDepth = 0.0f;
	float totalWeight = 0.0f;
    // �ֺ� �ȼ� ��ȸ (�簢�� ����)
    for (int y = -filterRadius; y <= filterRadius; ++y)
    {
        for (int x = -filterRadius; x <= filterRadius; ++x)
        {
            int2 neighborCoord = int2(centerCoord)+int2(x, y);

            // �ؽ�ó ���� üũ
            if (neighborCoord.x < 0 || neighborCoord.x >= width || neighborCoord.y < 0 || neighborCoord.y >= height)
            {
                continue;
            }

            // �̿� ���� �� �ε�
            float neighborDepth = DepthTexture.Load(int3(neighborCoord, 0));

            // ���� ����ġ ���
            float distSq = float(x * x + y * y);
            float weightSpatial = exp(-distSq / (2.0f * sigmaSpatial * sigmaSpatial));

            // ���� ����ġ ���
            float depthDiff = centerDepth - neighborDepth;
            float weightDepth = exp(-(depthDiff * depthDiff) / (2.0f * sigmaDepth * sigmaDepth));

            // ���� ����ġ
            float finalWeight = weightSpatial * weightDepth;

            totalWeightedDepth += neighborDepth * finalWeight;
            totalWeight += finalWeight;
        }
    }

    // ���� �� ��� (0���� ������ ����)
    float smoothedDepth = centerDepth; // �⺻���� ���� ����
    if (totalWeight > 1e-6f) // �Ӱ谪���� Ŭ ���� ���
    {
        smoothedDepth = totalWeightedDepth / totalWeight;
    }

    // ��� ����
    SmoothingDepthTexture[centerCoord] = smoothedDepth;
}