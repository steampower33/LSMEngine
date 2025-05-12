
#define GROUP_SIZE_X 16
#define GROUP_SIZE_Y 16
#define MAX_RADIUS 16

static const uint TILE_W = GROUP_SIZE_X + 2 * MAX_RADIUS;
static const uint TILE_H = GROUP_SIZE_Y + 2 * MAX_RADIUS;

Texture2D<float> DepthTexture : register(t2);

RWTexture2D<float> SmoothedDepth : register(u1);
//RWTexture2D<float4> SceneTexture : register(u2);

cbuffer RenderParams : register(b0)
{
    float4x4 invProj;
    float4x4 invView;

    int   filterRadius;
    float sigmaSpatial;
    float sigmaDepth;
    float shininess;

    uint width;
    float invWidth;
    uint height;
    float invHeight;

    float3 eyeWorld;
    float p1;

    float3 lightPos;
    float p2;

    float3 ambient;
    float p3;

    float3 diffuse;
    float p4;

    float3 specular;
    float p5;

    float4 p6;
};

groupshared float sharedDepth[TILE_H][TILE_W];

[numthreads(GROUP_SIZE_X, GROUP_SIZE_Y, 1)]
void main(uint3 gid : SV_GroupID,
    uint3 gtid : SV_GroupThreadID,
    uint3 dtid : SV_DispatchThreadID)
{
    int2 center = int2(dtid.xy);
    bool isValid = (center.x < int(width) && center.y < int(height));

    int2 tileOrigin = int2(int(gid.x) * GROUP_SIZE_X,
        int(gid.y) * GROUP_SIZE_Y)
        - int2(filterRadius, filterRadius);

    for (uint y = gtid.y; y < TILE_H; y += GROUP_SIZE_Y)
    {
        for (uint x = gtid.x; x < TILE_W; x += GROUP_SIZE_X)
        {
            int2 pos = tileOrigin + int2(x, y);

            // 범위 밖이면 far(1.0)로 채우기
            if (pos.x < 0 || pos.x >= int(width) ||
                pos.y < 0 || pos.y >= int(height))
            {
                sharedDepth[y][x] = 100.0;
            }
            else
            {
                sharedDepth[y][x] = DepthTexture.Load(int3(pos, 0));
            }
        }
    }

    GroupMemoryBarrierWithGroupSync();

    float centerD = sharedDepth[gtid.y + filterRadius]
        [gtid.x + filterRadius];

    if (centerD >= 100.0f) {
        SmoothedDepth[center] = centerD;
        return;
    }

    float sumD = 0.0f;
    float sumW = 0.0f;
    for (int oy = -filterRadius; oy <= filterRadius; oy++)
    {
        for (int ox = -filterRadius; ox <= filterRadius; ox++)
        {
            float neighD = sharedDepth[gtid.y + filterRadius + oy]
                [gtid.x + filterRadius + ox];
            float dist2 = float(ox * ox + oy * oy);
            float wS = exp(-dist2 / (2.0f * sigmaSpatial * sigmaSpatial));
            float diff = centerD - neighD;
            float wD = exp(-(diff * diff) / (2.0f * sigmaDepth * sigmaDepth));
            float w = wS * wD;
            sumD += neighD * w;
            sumW += w;
        }
    }

    float outD = (sumW > 1e-6f) ? sumD / sumW : centerD;

    GroupMemoryBarrierWithGroupSync();

    if (isValid)
    {
        SmoothedDepth[center] = outD;
        //SceneTexture[center] = float4(outD, outD, outD, 1);
    }
}