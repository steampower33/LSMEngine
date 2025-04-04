
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

struct PSInput
{
    float4 clipPos : SV_POSITION;
    float2 texCoord : TEXCOORD;
    float3 color : COLOR;
    uint primID : SV_PrimitiveID;
};

// https://en.wikipedia.org/wiki/Smoothstep
float smootherstep(float x, float edge0 = 0.0f, float edge1 = 1.0f)
{
    // Scale, and clamp x to 0..1 range
    x = clamp((x - edge0) / (edge1 - edge0), 0, 1);

    return x * x * x * (3 * x * (2 * x - 5) + 10.0f);
}

float CubicSpline(const float q)
{
    float coeff = 3.0f / (2.0f * 3.141592f);

    if (q < 1.0f)
        return coeff * (2.0f / 3.0f - q * q + 0.5f * q * q * q);
    else if (q < 2.0f)
        return coeff * pow(2.0f - q, 3.0f) / 6.0f;
    else // q >= 2.0f
        return 0.0f;
}

float4 main(PSInput input) : SV_TARGET
{
    float radius = 0.5;
    float dist = length(float2(0.5, 0.5) - input.texCoord);

    if (dist > radius)
        discard;

    //float q = distFromCenter * 2.0;
    //float scale = CubicSpline(q * 2);

    float q = dist / radius;
    float scale = 1.0 - q;

    return float4(input.color.rgb * scale, 1.0);
}
