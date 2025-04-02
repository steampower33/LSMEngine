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
	struct SimParams {
		float deltaTime = 0.0f;
		XMFLOAT2 gravity = XMFLOAT2(0.0f, -9.8f); // �߷�
		uint32_t numParticles = 0;

		XMFLOAT2 minBounds = XMFLOAT2(-5.0f, -5.0f); // ��� �ּҰ�
		XMFLOAT2 maxBounds = XMFLOAT2(5.0f, 5.0f);   // ��� �ִ밪

		float boundaryDamping = 0.6f;
		float padding1;
		float padding2;
		float padding3;
	};

	void Initialize(ComPtr<ID3D12Device> device,
		ComPtr<ID3D12GraphicsCommandList> commandList);

private:
	uint32_t maxParticles = 50;

	// Ping Pong �������� ���� ����
	// �� ������ ���� ���� �д� ���۸� ����
	// ���۴� srv, uav �ϳ���
	ComPtr<ID3D12Resource> m_particleBuffer[2];

	// SRV UAV | SRV UAV | CBV
	ComPtr<ID3D12DescriptorHeap> m_srvUavHeap;
	UINT m_srvUavDescriptorSize = 0;

	D3D12_CPU_DESCRIPTOR_HANDLE m_particleBufferSrvCpuHandle[2];
	D3D12_GPU_DESCRIPTOR_HANDLE m_particleBufferSrvGpuHandle[2];
	D3D12_CPU_DESCRIPTOR_HANDLE m_particleBufferUavCpuHandle[2];
	D3D12_GPU_DESCRIPTOR_HANDLE m_particleBufferUavGpuHandle[2];

	UINT m_currentReadBufferIndex = 0;

};