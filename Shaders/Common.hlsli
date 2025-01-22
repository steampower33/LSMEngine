
#define MAX_LIGHTS 3 // 쉐이더에서도 #define 사용 가능
#define NUM_DIR_LIGHTS 1
#define NUM_POINT_LIGHTS 1
#define NUM_SPOT_LIGHTS 1

struct Material
{
    float3 albedo;
    float metallic;
    float roughness;
};

// 조명
struct Light
{
    float3 radiance;
    float fallOffStart;
    float3 direction;
    float fallOffEnd;
    float3 position;
    float spotPower;
    float3 d1;
    float d2;
};

cbuffer GlobalConstants : register(b0)
{
    float4x4 view;
    float4x4 proj;
    float4x4 d01;
    float4x4 d02;
    
    Light light[MAX_LIGHTS];
    float3 eyeWorld;
    float d03[13];
}

cbuffer MeshConstants : register(b1)
{
    float4x4 world;
    
    float4x4 worldIT;
    
    Material material;
    float heightScale;
    uint useAlbedoMap;
    uint useNormalMap;
    uint useHeightMap;
    uint useAOMap;
    uint useMetallicMap;
    uint useRoughnessMap;
    uint useEmissiveMap;
    uint invertNormalMapY;
    float d10[2];
    
    float4x4 d11;
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
    float d21[7];
    
    float4x4 d22;
    
    float4x4 d23;
    
    float4x4 d24;
}

cbuffer CubemapIndexConstants : register(b3)
{
    uint cubemapEnvIndex;
    uint cubemapDiffuseIndex;
    uint cubemapSpecularIndex;
    uint brdfIndex;
    float d31[12];
    
    float4x4 d32;
    
    float4x4 d33;
    
    float4x4 d34;
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
    uint d40[8];
    
    float4x4 d42;
    
    float4x4 d43;
    
    float4x4 d44;
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