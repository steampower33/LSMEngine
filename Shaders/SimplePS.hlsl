#include "Common.hlsli" // ���̴������� include ��� ����

float4 main(PSInput input) : SV_TARGET
{
    return float4(albedoFactor, 1.0);
}
