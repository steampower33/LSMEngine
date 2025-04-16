
#include "SphCommon.hlsli" // ParticleHash ���� ���� ����

RWStructuredBuffer<ParticleHash> DataBuffer : register(u0);

cbuffer SortParams : register(b0)
{
    uint numElements;
};

groupshared ParticleHash sharedData[GROUP_SIZE_X];

void SharedMemoryCompareAndSwap(uint index1, uint index2, bool sortAscending)
{
    if (index1 >= GROUP_SIZE_X || index2 >= GROUP_SIZE_X) return;

    ParticleHash hash1 = sharedData[index1];
    ParticleHash hash2 = sharedData[index2];

    // sortAscending == true: �������� ����
    // sortAscending == false: �������� ����
    bool shouldSwap = (hash1.cellKey > hash2.cellKey);
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

    if (globalIndex < numElements)
    {
        sharedData[localIndex] = DataBuffer[globalIndex];
    }
    else
    {
        // ���⼭�� ������ cellKey�� �ſ� ū ������ ���� (�������� ����)
        sharedData[localIndex].cellKey = 0xFFFFFFFF; // �Ǵ� ������ �ִ밪
        sharedData[localIndex].particleID = 0xFFFFFFFF;
    }

    GroupMemoryBarrierWithGroupSync();

    // k: ������ �������� ũ�� (2, 4, 8, ..., GROUP_SIZE_X)
    for (uint k = 2; k <= GROUP_SIZE_X; k *= 2)
    {
        // ���� �����尡 �����ؾ� �� ���� ���� (Bitonic Merge �ܰ�)
        // (i & k) == 0 �κа� ������ ���� -> ������ �ε����� k ũ���� ������� ������ ���� ����
        bool sortAscending = ((localIndex & k) == 0);

        // j: ���� ��� ���� �Ÿ� (k/2, k/4, ..., 1)
        for (uint j = k / 2; j > 0; j /= 2)
        {
            // ���� ���� �ε��� ���
            uint partnerIndex = localIndex ^ j; // XOR �������� ��Ʈ�� ã��

            // ��Ʈ�ʰ� �ڱ� �ڽź��� �ڿ� �ִ� ��쿡�� �� ���� (�ߺ� ����)
            // ���� ��Ʈ�ʰ� ���� k-��� ���� �ִ��� Ȯ�� (���� ��Ʈ�� ���ƾ� ��) -> �� �κ��� j < k/2 ���� �ڿ������� ����
            if (partnerIndex > localIndex)
            {
                SharedMemoryCompareAndSwap(localIndex, partnerIndex, sortAscending);
            }

            GroupMemoryBarrierWithGroupSync();
        }
    }

    if (globalIndex < numElements)
    {
        DataBuffer[globalIndex] = sharedData[localIndex];
    }
}