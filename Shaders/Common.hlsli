
#define MAX_LIGHTS 1 // 쉐이더에서도 #define 사용 가능
#define NUM_DIR_LIGHTS 1
#define NUM_POINT_LIGHTS 1
#define NUM_SPOT_LIGHTS 1

// 재질
struct Material
{
    float3 ambient;
    float shininess;
    float3 diffuse;
    float dummy1;
    float3 specular;
    float dummy2;
    float3 dummy3;
    float dummy4;
};

// 조명
struct Light
{
    float3 strength;
    float fallOffStart;
    float3 direction;
    float fallOffEnd;
    float3 position;
    float spotPower;
    float3 dummy1;
    float dummy2;
};

float3 BlinnPhong(float3 lightStrength, float3 lightVec, float3 normal,
                   float3 toEye, Material mat)
{
    // TODO:
    float3 halfway = normalize(lightVec + -toEye);
    float ndoth = max(dot(normal, halfway), 0.0);
    float3 specualr = mat.specular * pow(ndoth, mat.shininess);
    
    return mat.ambient + (mat.diffuse + specualr) * lightStrength;

}

float3 ComputeDirectionalLight(Light L, Material mat, float3 normal,
                                float3 toEye)
{
    // TODO:
    float3 lightVec = -L.direction;
    
    float ndotl = max(dot(normal, lightVec), 0.0f);
    float3 lightStrength = L.strength * ndotl;
    return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);

}

cbuffer GlobalConstants : register(b0)
{
    float4x4 view;
    
    float4x4 proj;
    
    Light lights[MAX_LIGHTS];
    
    float3 eyeWorld;
    bool isUseTexture;
    bool d1[3];
    float4x3 dummy;
}

cbuffer MeshConstants : register(b1)
{
    float4x4 world;
    
    float4x4 worldIT;
    
    Material material;
    
    float4x4 d2;
}

cbuffer TextureIndexConstants : register(b2)
{
    uint diffuseIndex;
    uint cubemapIndex;
    float d3[14];
    
    float4x4 d4;
    
    float4x4 d5;
    
    float4x4 d6;
}

struct VSInput
{
    float3 posModel : POSITION;
    float2 texcoord : TEXCOORD;
    float3 normalModel : NORMAL;
};

struct PSInput
{
    float3 posWorld : POSITION;
    float4 posProj : SV_POSITION;
    float3 normalWorld : NORMAL;
    float2 texcoord : TEXCOORD;
};