Texture2D<float4> inputTex : register(t0);
RWTexture2D<float4> outputTex : register(u0);

static const float weights[11] =
{
    0.05f, 0.05f, 0.1f, 0.1f, 0.1f, 0.2f, 0.1f, 0.1f, 0.1f, 0.05f, 0.05f,
};

static const int blurRadius = 5;

#define N 256
#define CACHE_SIZE (N + 2*blurRadius)

groupshared float4 groupCache[CACHE_SIZE];

[numthreads(1, N, 1)]
void main(int3 gID : SV_GroupID, int3 gtID : SV_GroupThreadID,
          uint3 dtID : SV_DispatchThreadID)
{
    uint width, height;
    outputTex.GetDimensions(width, height);

    if (gtID.y < blurRadius)
    {
        int y = max(int(dtID.y) - blurRadius, 0);
        groupCache[gtID.y] = inputTex[int2(dtID.x, y)];
    }
    
    if (gtID.y >= N - blurRadius)
    {
        int y = min(dtID.y + blurRadius, height - 1);
        groupCache[gtID.y + 2 * blurRadius] = inputTex[int2(dtID.x, y)];
    }
    
    groupCache[gtID.y + blurRadius] =
        inputTex[min(dtID.xy, uint2(width, height) - 1)];

    GroupMemoryBarrierWithGroupSync();
    
    float4 blurColor = float4(0, 0, 0, 0);

    //[unroll]
    for (int i = -blurRadius; i <= blurRadius; ++i)
    {
        int k = gtID.y + blurRadius + i;
        blurColor += weights[i + blurRadius] * groupCache[k];
    }

    outputTex[dtID.xy] = blurColor;
}
