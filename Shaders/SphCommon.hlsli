struct Particle {
    float3 position;
    float size;
    float3 velocity;
    float life;
    float3 color;
    float density;
    float3 force;
    float pressure;
};

struct ParticleHash
{
    uint particleID; // ���� ��ƼŬ �ε���
    uint hashValue;  // ���� �ؽ� ��
    uint flag;       // �׷� Flag  
};

struct ScanResult
{
    uint groupID;
};

struct CompactCell
{
    uint hashValue;
    uint startIndex;
    uint endIndex;
};

#ifndef GROUP_SIZE_X
    #define GROUP_SIZE_X 512
#endif

#ifndef MAX_PARTICLES
    #define MAX_PARTICLES 1024
#endif