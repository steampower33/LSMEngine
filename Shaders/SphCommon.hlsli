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
    uint particleID; // 원래 파티클 인덱스
    uint hashValue;  // 계산된 해시 값
    uint flag;       // 그룹 Flag  
};

struct ScanResult
{
    uint groupID;
};

#ifndef GROUP_SIZE_X
    #define GROUP_SIZE_X 256
#endif

#ifndef MAX_PARTICLES
    #define MAX_PARTICLES 256
#endif