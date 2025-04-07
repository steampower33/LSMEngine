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
    uint particleID; // 원래 파티클 인덱스
    uint hashValue;  // 계산된 해시 값
    uint flag;       // 그룹 Flag  
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