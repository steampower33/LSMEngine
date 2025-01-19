
#define MAX_LIGHTS 3 // ���̴������� #define ��� ����
#define NUM_DIR_LIGHTS 1
#define NUM_POINT_LIGHTS 1
#define NUM_SPOT_LIGHTS 1

// ����
struct Material
{
    float color;
    float diffuse;
    float specular;
    float shininess;
};

// ����
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

float CalcAttenuation(float d, float falloffStart, float falloffEnd)
{
    // Linear falloff
    return saturate((falloffEnd - d) / (falloffEnd - falloffStart));
}

float BlinnPhong(float3 lightStrength, float3 lightVec, float3 normal,
                   float3 toEye, Material mat)
{
    float3 halfway = normalize(lightVec + -toEye);
    float ndoth = max(dot(normal, halfway), 0.0);
    float specualr = mat.specular * pow(ndoth, mat.shininess);
    
    return mat.color + length((mat.diffuse + specualr) * lightStrength) / 3;

}

float ComputeDirectionalLight(Light L, Material mat, float3 normal,
                                float3 toEye)
{
    float3 lightVec = -L.direction;
    
    float ndotl = max(dot(normal, lightVec), 0.0f);
    float3 lightStrength = L.strength * ndotl;
    return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);

}

float ComputePointLight(Light L, Material mat, float3 pos, float3 normal,
                          float3 toEye)
{
    float3 lightVec = L.position - pos;

    // ���̵��� �������� ��������� �Ÿ� ���
    float d = length(lightVec);

    // �ʹ� �ָ� ������ ������� ����
    if (d > L.fallOffEnd)
    {
        return 0.0;
    }
    else
    {
        lightVec /= d;
        
        float ndotl = max(dot(lightVec, normal), 0.0f);
        float3 lightStrength = L.strength * ndotl;
        
        float att = CalcAttenuation(d, L.fallOffStart, L.fallOffEnd);
        lightStrength *= att;
        
        float spotFactor = pow(max(dot(-lightVec, L.direction), 0.0f), L.spotPower);
        lightStrength *= spotFactor;
        
        return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);

    }
}

float ComputeSpotLight(Light L, Material mat, float3 pos, float3 normal,
                         float3 toEye)
{
    float3 lightVec = L.position - pos;

    // ���̵��� �������� ��������� �Ÿ� ���
    float d = length(lightVec);

    // �ʹ� �ָ� ������ ������� ����
    if (d > L.fallOffEnd)
    {
        return 0.0;
    }
    else
    {
        // TODO:
        lightVec /= d;
        
        float ndotl = max(dot(lightVec, normal), 0.0f);
        float3 lightStrength = L.strength * ndotl;
        
        float att = CalcAttenuation(d, L.fallOffStart, L.fallOffEnd);
        lightStrength *= att;
        
        float spotFactor = pow(max(dot(-lightVec, L.direction), 0.0f), L.spotPower);
        lightStrength *= spotFactor;
        
        return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);
    }
    
    // if�� else�� ���� ��� ��� �߻�
    // warning X4000: use of potentially uninitialized variable
}

cbuffer GlobalConstants : register(b0)
{
    float4x4 view;
    float4x4 proj;
    float4x4 d01;
    float4x4 d02;
    
    Light lights[MAX_LIGHTS];
    float3 eyeWorld;
    float d03[13];
}

cbuffer MeshConstants : register(b1)
{
    float4x4 world;
    
    float4x4 worldIT;
    
    Material material;
    uint isUseTexture;
    uint isUseNormalMap;
    uint isUseHeightMap;
    float heightScale;
    float d10[8];
    
    float4x4 d11;
}

cbuffer TextureIndexConstants : register(b2)
{
    uint albedoIndex;
    uint diffuseIndex;
    uint specularIndex;
    uint normalIndex;
    uint heightIndex;
    float d21[11];
    
    float4x4 d22;
    
    float4x4 d23;
    
    float4x4 d24;
}

cbuffer CubemapIndexConstants : register(b3)
{
    uint cubemapEnvIndex;
    uint cubemapDiffuseIndex;
    uint cubemapSpecularIndex;
    float d31[13];
    
    float4x4 d32;
    
    float4x4 d33;
    
    float4x4 d34;
}

cbuffer SamplingConstants : register(b4)
{
    float dx;
    float dy;
    float threshold;
    float strength;
    
    uint index;
    uint hightIndex;
    uint lowIndex;
    uint d40[9];
    
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