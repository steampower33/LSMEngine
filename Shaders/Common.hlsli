#ifndef __COMMON_HLSLI__
#define __COMMON_HLSLI__

#define MAX_LIGHTS 2
#define LIGHT_OFF 0x00
#define LIGHT_DIRECTIONAL 0x01
#define LIGHT_POINT 0x02
#define LIGHT_SPOT 0x04
#define LIGHT_SHADOW 0x10

SamplerState linearWrapSampler : register(s0, space0);
SamplerState linearClampSampler : register(s1, space0);
SamplerState shadowPointSampler : register(s2, space0);
SamplerComparisonState shadowCompareSampler : register(s3, space0);

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

// ����
struct Light
{
    float4x4 viewProj; // �׸��� �������� �ʿ�
    float4x4 invProj; // �׸��� ������ ������

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
    float d00;
    float d01;
    float d02;
    
    float4x4 d03;
    float4x4 d04;
    float4x4 d05;
    
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

static const float2 poissonDisk[16] =
{
    float2(-0.94201624, -0.39906216), float2(0.94558609, -0.76890725),
            float2(-0.094184101, -0.92938870), float2(0.34495938, 0.29387760),
            float2(-0.91588581, 0.45771432), float2(-0.81544232, -0.87912464),
            float2(-0.38277543, 0.27676845), float2(0.97484398, 0.75648379),
            float2(0.44323325, -0.97511554), float2(0.53742981, -0.47373420),
            float2(-0.26496911, -0.41893023), float2(0.79197514, 0.19090188),
            float2(-0.24188840, 0.99706507), float2(-0.81409955, 0.91437590),
            float2(0.19984126, 0.78641367), float2(0.14383161, -0.14100790)
};
        
static const float2 diskSamples64[64] =
{
    float2(0.0, 0.0),
            float2(-0.12499844227275288, 0.000624042775189866), float2(0.1297518688031755, -0.12006020382326336),
            float2(-0.017851253586055427, 0.21576916541852392), float2(-0.1530983013115895, -0.19763833164521946),
            float2(0.27547541035593626, 0.0473106572479027), float2(-0.257522587854559, 0.16562643733622642),
            float2(0.0842605283808073, -0.3198048832600703), float2(0.1645196099088727, 0.3129429627830483),
            float2(-0.3528833088400373, -0.12687935349026194), float2(0.36462214742013344, -0.1526456341030772),
            float2(-0.17384046457324884, 0.37637015407303087), float2(-0.1316547617859344, -0.4125130588224921),
            float2(0.3910687393754993, 0.2240317858770442), float2(-0.45629121277761536, 0.10270505898899496),
            float2(0.27645268679640483, -0.3974278701387824), float2(0.06673001731984558, 0.49552709793561556),
            float2(-0.39574431915605623, -0.33016879600548193), float2(0.5297612167716342, -0.024557141621887494),
            float2(-0.3842909284448636, 0.3862583103507092), float2(0.0230336562454131, -0.5585422550532486),
            float2(0.36920334463249477, 0.43796562686149154), float2(-0.5814490172413539, -0.07527974727019048),
            float2(0.4903718680780365, -0.3448339179919178), float2(-0.13142003698572613, 0.5981043168868373),
            float2(-0.31344141845114937, -0.540721256470773), float2(0.608184438565748, 0.19068741092811003),
            float2(-0.5882602609696388, 0.27536315179038107), float2(0.25230610046544444, -0.6114259003901626),
            float2(0.23098706800827415, 0.6322736546883326), float2(-0.6076303951666067, -0.31549215975943595),
            float2(0.6720886334230931, -0.1807536135834609), float2(-0.37945598830371974, 0.5966683776943834),
            float2(-0.1251555455510758, -0.7070792667147104), float2(0.5784815570900413, 0.44340623372555477),
            float2(-0.7366710399837763, 0.0647362251696953), float2(0.50655463562529, -0.553084443034271),
            float2(8.672987356252326e-05, 0.760345311340794), float2(-0.5205650355786364, -0.5681215043747359),
            float2(0.7776435491294021, 0.06815798190547596), float2(-0.6273416101921778, 0.48108471615868836),
            float2(0.1393236805531513, -0.7881712453757264), float2(0.4348773806743975, 0.6834703093608201),
            float2(-0.7916014213464706, -0.21270211499241704), float2(0.7357897682897174, -0.38224784745000717),
            float2(-0.2875567908732709, 0.7876776574352392), float2(-0.3235695699691864, -0.7836151691933712),
            float2(0.7762165924462436, 0.3631291803355136), float2(-0.8263007976064866, 0.2592816844184794),
            float2(0.4386452756167397, -0.7571098481588484), float2(0.18988542402304126, 0.8632459242554175),
            float2(-0.7303253445407815, -0.5133224046555819), float2(0.8939004035324556, -0.11593993515830946),
            float2(-0.5863762307291154, 0.6959079795748251), float2(-0.03805753378232556, -0.9177699189461416),
            float2(0.653979655650389, 0.657027860897389), float2(-0.9344208130797295, -0.04310155546401203),
            float2(0.7245109901504777, -0.6047386420191574), float2(-0.12683493131695708, 0.9434844461875473),
            float2(-0.5484582700240663, -0.7880790100251422), float2(0.9446610338564589, 0.2124041692463835),
            float2(-0.8470120123194587, 0.48548496473788055), float2(0.29904134279525085, -0.9377229203230629),
            float2(0.41623562331748715, 0.9006236205438447),
};

#endif // __COMMON_HLSLI__
