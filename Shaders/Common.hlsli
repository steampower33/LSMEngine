
#define MAX_LIGHTS 1
#define LIGHT_OFF 0x00
#define LIGHT_DIRECTIONAL 0x01
#define LIGHT_POINT 0x02
#define LIGHT_SPOT 0x04
#define LIGHT_SHADOW 0x10

// 조명
struct Light
{
    float4x4 viewProj; // 그림자 렌더링에 필요
    float4x4 invProj; // 그림자 렌더링 디버깅용

    float3 radiance;
    float fallOffStart;
    float3 direction;
    float fallOffEnd;
    float3 position;
    float spotPower;
    uint type;
    float radius;
    float d0;
    float d1;
    
    float4x4 d2;
};

cbuffer GlobalConstants : register(b0)
{
    float4x4 view;
    float4x4 proj;
    float4x4 invProj;
    
    float3 eyeWorld;
    float strengthIBL;
    
    int choiceEnvMap;
    float envLodBias;
    int mode;
    float depthScale;
    
    float fogStrength;
    uint depthOnlySRVIndex;
    uint shadowDepthOnlyIndex;
    uint resolvedSRVIndex;
    
    uint fogSRVIndex;
    uint shadowMapSRVIndex;
    float d00;
    float d01;
    
    Light light[MAX_LIGHTS];
}

cbuffer MeshConstants : register(b1)
{
    float4x4 world;
    
    float4x4 worldIT;
    
    float3 albedoFactor;
    float metallicFactor;
    float roughnessFactor;
    float3 emissionFactor;
    float heightScale;
    uint useAlbedoMap;
    uint useNormalMap;
    uint useHeightMap;
    uint useAOMap;
    uint useMetallicMap;
    uint useRoughnessMap;
    uint useEmissiveMap;
    
    uint invertNormalMapY;
    float meshLodBias;
    float d10;
    float d11;
    float4x3 d12;
}

cbuffer TextureIndexConstants : register(b2)
{
    uint albedoIndex;
    uint diffuseIndex;
    uint specularIndex;
    uint normalIndex;
    
    uint heightIndex;
    uint aoIndex;
    uint metallicIndex;
    uint roughnessIndex;
    
    uint emissiveIndex;
    float d20;
    float d21;
    float d22;
    
    float4 d23;
    
    float4x4 d24;
    
    float4x4 d25;
    
    float4x4 d26;
}

cbuffer CubemapIndexConstants : register(b3)
{
    uint cubemapEnvIndex;
    uint cubemapDiffuseIndex;
    uint cubemapSpecularIndex;
    uint brdfIndex;
    
    float d30[12];
    
    float4x4 d31;
    
    float4x4 d32;
    
    float4x4 d33;
}

cbuffer SamplingConstants : register(b4)
{
    float dx;
    float dy;
    float strength;
    float exposure;
    
    float gamma;
    uint index;
    uint hightIndex;
    uint lowIndex;
    
    float4 d40[2];
    
    float4x4 d41;
    
    float4x4 d42;
    
    float4x4 d43;
}

struct VSInput
{
    float3 posModel : POSITION;
    float2 texcoord : TEXCOORD;
    float3 normalModel : NORMAL;
    float3 tangentModel : TANGENT;
};

struct PSInput
{
    float3 posWorld : POSITION;
    float4 posProj : SV_POSITION;
    float3 normalWorld : NORMAL;
    float3 tangentWorld : TANGENT;
    float2 texcoord : TEXCOORD;
};