
#define MAX_LIGHTS 3 // ���̴������� #define ��� ����
#define NUM_DIR_LIGHTS 1
#define NUM_POINT_LIGHTS 1
#define NUM_SPOT_LIGHTS 1

// ����
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

float3 BlinnPhong(float3 lightStrength, float3 lightVec, float3 normal,
                   float3 toEye, Material mat)
{
    float3 halfway = normalize(lightVec + -toEye);
    float ndoth = max(dot(normal, halfway), 0.0);
    float3 specualr = mat.specular * pow(ndoth, mat.shininess);
    
    return mat.ambient + (mat.diffuse + specualr) * lightStrength;

}

float3 ComputeDirectionalLight(Light L, Material mat, float3 normal,
                                float3 toEye)
{
    float3 lightVec = -L.direction;
    
    float ndotl = max(dot(normal, lightVec), 0.0f);
    float3 lightStrength = L.strength * ndotl;
    return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);

}

float3 ComputePointLight(Light L, Material mat, float3 pos, float3 normal,
                          float3 toEye)
{
    float3 lightVec = L.position - pos;

    // ���̵��� �������� ��������� �Ÿ� ���
    float d = length(lightVec);

    // �ʹ� �ָ� ������ ������� ����
    if (d > L.fallOffEnd)
    {
        return float3(0.0, 0.0, 0.0);
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

float3 ComputeSpotLight(Light L, Material mat, float3 pos, float3 normal,
                         float3 toEye)
{
    float3 lightVec = L.position - pos;

    // ���̵��� �������� ��������� �Ÿ� ���
    float d = length(lightVec);

    // �ʹ� �ָ� ������ ������� ����
    if (d > L.fallOffEnd)
    {
        return float3(0.0f, 0.0f, 0.0f);
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
    float4x4 d11;
    float4x4 d12;
    
    Light lights[MAX_LIGHTS];
    
    float3 eyeWorld;
    bool isUseTexture;
    bool d13[3];
    float4x3 dummy;
}

cbuffer MeshConstants : register(b1)
{
    float4x4 world;
    
    float4x4 worldIT;
    
    Material material;
    
    float4x4 d21;
}

cbuffer TextureIndexConstants : register(b2)
{
    uint ambientIndex;
    uint diffuseIndex;
    uint specularIndex;
    float d31[13];
    
    float4x4 d32;
    
    float4x4 d33;
    
    float4x4 d34;
}

cbuffer CubemapIndexConstants : register(b3)
{
    uint cubemapAmbientIndex;
    uint cubemapDiffuseIndex;
    uint cubemapSpecularIndex;
    float d41[13];
    
    float4x4 d42;
    
    float4x4 d43;
    
    float4x4 d44;
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