#pragma once

#include <iostream>

#include <d3d12.h>
#include "d3dx12.h"
#include <dxgi1_6.h>
#include <DirectXMath.h>

#include "Helpers.h"
#include "ConstantBuffers.h"
#include "GraphicsCommon.h"

#include <numeric>
#include <random>

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
		UINT numParticles = 128;

		XMFLOAT2 minBounds = XMFLOAT2(-5.0f, -5.0f); // ��� �ּҰ�
		XMFLOAT2 maxBounds = XMFLOAT2(5.0f, 5.0f);   // ��� �ִ밪
	};

	void Initialize(ComPtr<ID3D12Device> device,
		ComPtr<ID3D12GraphicsCommandList> commandList, UINT width, UINT height);
	void Update(float dt);
	void Render(ComPtr<ID3D12GraphicsCommandList>& commandList);

private:
	vector<Particle> m_particlesCPU;
	UINT m_maxParticles = 128;

	// SRV UAV | SRV UAV | CBV
	ComPtr<ID3D12DescriptorHeap> m_heap;
	UINT m_cbvSrvUavSize = 0;

	// Ping Pong �������� ���� ����(Particle Structured)
	// �� ������ ���� ���� �д� ���۸� ����
	// ���۴� srv, uav �ϳ���
	ComPtr<ID3D12Resource> m_particleBuffer[2];
	ComPtr<ID3D12Resource> m_uploadBuffer;
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_particleBufferSrvGpuHandle[2];
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_particleBufferUavGpuHandle[2];

	ComPtr<ID3D12Resource> m_constantBuffer;
	SimParams m_constantBufferData;
	UINT8* m_constantBufferDataBegin = nullptr;
	UINT m_constantBufferSize = (sizeof(SimParams) + 255) & ~255;
	D3D12_CPU_DESCRIPTOR_HANDLE m_constantBufferCbvCpuHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_constantBufferCbvGpuHandle;

	// �ʱ⿡ Update�Լ��� ���� ��ġ�⿡ read�� 0��, write�� 1������ ���� �Ҽ� �ֵ��� �ʱ⿡ ����
	UINT m_readIdx = 1;
	UINT m_writeIdx = 0;

	void GenerateParticles();
	void CreateStructuredBuffer(ComPtr<ID3D12Device>& device, UINT width, UINT height, UINT index);
	void UploadAndCopyParticleData(ComPtr<ID3D12Device> device,
		ComPtr<ID3D12GraphicsCommandList> commandList);
};