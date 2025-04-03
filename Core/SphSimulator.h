#pragma once

#include <iostream>

#include <d3d12.h>
#include "d3dx12.h"
#include <dxgi1_6.h>
#include <DirectXMath.h>

#include "Helpers.h"
#include "ConstantBuffers.h"
#include "GraphicsCommon.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;
using namespace std;

class SphSimulator
{
public:
	SphSimulator();
	~SphSimulator();

	// ���� ����
	struct Particle {
		XMFLOAT2 position = XMFLOAT2(0.0f, 0.0f);
		XMFLOAT2 velocity = XMFLOAT2(0.0f, 0.0f);

		XMFLOAT4 color = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);

		float life = 0.0f;
		float size = 1.0f;
		float padding1;
		float padding2;
	};

	// �ùķ��̼� �Ķ���� (��� ���ۿ�)
	__declspec(align(256)) struct SimParams {
		float deltaTime = 0.0f;
		XMFLOAT2 gravity = XMFLOAT2(0.0f, -9.8f); // �߷�
		UINT numParticles = 0;

		XMFLOAT2 minBounds = XMFLOAT2(-5.0f, -5.0f); // ��� �ּҰ�
		XMFLOAT2 maxBounds = XMFLOAT2(5.0f, 5.0f);   // ��� �ִ밪
	};

	void Initialize(ComPtr<ID3D12Device> device,
		ComPtr<ID3D12GraphicsCommandList> commandList, UINT width, UINT height);

private:
	UINT m_maxParticles = 50;

	// Ping Pong �������� ���� ����(Particle Structured)
	// �� ������ ���� ���� �д� ���۸� ����
	// ���۴� srv, uav �ϳ���
	ComPtr<ID3D12Resource> m_particleBuffer[2];

	// SRV UAV | SRV UAV | CBV
	ComPtr<ID3D12DescriptorHeap> m_heap;
	UINT m_cbvSrvUavSize = 0;

	ComPtr<ID3D12Resource> m_constsBuffer;
	GlobalConstants m_constsBufferData;
	UINT8* m_constsBufferDataBegin = nullptr;

	UINT m_currentReadBufferIndex = 0;

	void CreateStructuredBuffer(ComPtr<ID3D12Device>& device, UINT width, UINT height, UINT index);
};