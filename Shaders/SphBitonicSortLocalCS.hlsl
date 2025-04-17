
#include "SphCommon.hlsli" // ParticleHash ���� ���� ����

RWStructuredBuffer<ParticleHash> hashes : register(u0);

groupshared ParticleHash sharedData[GROUP_SIZE_X];

void SharedMemoryCompareAndSwap(uint index1, uint index2, bool sortAscending)
{
    if (index1 >= GROUP_SIZE_X || index2 >= GROUP_SIZE_X) return;

    ParticleHash hash1 = sharedData[index1];
    ParticleHash hash2 = sharedData[index2];

    // sortAscending == true: �������� ����
    // sortAscending == false: �������� ����
    bool shouldSwap = (hash1.cellIndex > hash2.cellIndex);
    if (!sortAscending)
    {
        shouldSwap = !shouldSwap; // ���������̸� ���� ���� (������ �ڷ�)
    }

    if (shouldSwap)
    {
        sharedData[index1] = hash2;
        sharedData[index2] = hash1;
    }
}

[numthreads(GROUP_SIZE_X, 1, 1)]
void main(uint3 gtid : SV_DispatchThreadID,
    uint3 gidx : SV_GroupID,
    uint3 dtid : SV_GroupThreadID)
{
    uint globalIndex = gtid.x; // ���� �����尡 ó���� �۷ι� �ε���
    uint localIndex = dtid.x;  // ���� �������� �׷� �� ���� �ε���

    if (globalIndex < numParticles)
    {
        sharedData[localIndex] = hashes[globalIndex];
    }
    else
    {
        sharedData[localIndex].cellIndex = 0x7FFFFFFF; // int max
        sharedData[localIndex].particleID = 0xFFFFFFFF;
        sharedData[localIndex].flag = 0; // �ʿ�� �ʱ�ȭ
    }

    GroupMemoryBarrierWithGroupSync();

    int particleCnt = (numParticles < GROUP_SIZE_X) ? numParticles : GROUP_SIZE_X;

    // k: ������ �������� ũ�� (2, 4, 8, ..., GROUP_SIZE_X)
    for (uint k = 2; k <= GROUP_SIZE_X; k *= 2) { // ���� ������ GROUP_SIZE_X ����
        bool sortAscending = ((localIndex & k) == 0);
        for (uint j = k / 2; j > 0; j /= 2) {
            uint partnerIndex = localIndex ^ j;
            if (partnerIndex > localIndex) {
                SharedMemoryCompareAndSwap(localIndex, partnerIndex, sortAscending);
            }
            GroupMemoryBarrierWithGroupSync(); // �� j �ܰ� �� ����ȭ
        }
    }

    if (globalIndex < numParticles)
    {
        hashes[globalIndex] = sharedData[localIndex];
    }
}