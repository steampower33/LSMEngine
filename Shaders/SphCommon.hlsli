struct Particle {
    float3 position;
    float p1;
    float3 velocity;
    float p2;
    float3 color;
    float p3;
    float size;
    float life;
    float p4;
    float p5;
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