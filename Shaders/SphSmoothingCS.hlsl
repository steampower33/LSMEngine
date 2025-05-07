
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
    // 주변 픽셀 순회 (사각형 영역)
    for (int y = -filterRadius; y <= filterRadius; ++y)
    {
        for (int x = -filterRadius; x <= filterRadius; ++x)
        {
            int2 neighborCoord = int2(centerCoord)+int2(x, y);

            // 텍스처 범위 체크
            if (neighborCoord.x < 0 || neighborCoord.x >= width || neighborCoord.y < 0 || neighborCoord.y >= height)
            {
                continue;
            }

            // 이웃 깊이 값 로드
            float neighborDepth = DepthTexture.Load(int3(neighborCoord, 0));

            // 공간 가중치 계산
            float distSq = float(x * x + y * y);
            float weightSpatial = exp(-distSq / (2.0f * sigmaSpatial * sigmaSpatial));

            // 깊이 가중치 계산
            float depthDiff = centerDepth - neighborDepth;
            float weightDepth = exp(-(depthDiff * depthDiff) / (2.0f * sigmaDepth * sigmaDepth));

            // 최종 가중치
            float finalWeight = weightSpatial * weightDepth;

            totalWeightedDepth += neighborDepth * finalWeight;
            totalWeight += finalWeight;
        }
    }

    // 최종 값 계산 (0으로 나누기 방지)
    float smoothedDepth = centerDepth; // 기본값은 원본 깊이
    if (totalWeight > 1e-6f) // 임계값보다 클 때만 계산
    {
        smoothedDepth = totalWeightedDepth / totalWeight;
    }

    // 결과 쓰기
    SmoothingDepthTexture[centerCoord] = smoothedDepth;
}