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
		XMFLOAT3 position = XMFLOAT3(0.0f, 0.0f, 0.0f);
		float p1;
		XMFLOAT3 velocity = XMFLOAT3(0.0f, 0.0f, 0.0f);
		float p2;
		XMFLOAT3 color = XMFLOAT3(0.0f, 0.0f, 0.0f);
		float p3;
		float size = 1.0f;
		float life = 0.0f;
		float p4;
		float p5;
	};

	// �ùķ��̼� �Ķ���� (��� ���ۿ�)
	__declspec(align(256)) struct SimParams {
		float deltaTime = 0.0f;
		XMFLOAT2 gravity = XMFLOAT2(0.0f, -9.8f); // �߷�
		UINT numParticles = 128;

		XMFLOAT3 minBounds = XMFLOAT3(-1.0f, -1.0f, 0.0f); // ��� �ּҰ�
		float d1;
		XMFLOAT3 maxBounds = XMFLOAT3(1.0f, 1.0f, 0.0f);   // ��� �ִ밪
		float d2;
	};

	float minBounds[3] = { -3.0f, -3.0f, 0.0f };
	float maxBounds[3] = { 3.0f, 3.0f, 0.0f };

	void Initialize(ComPtr<ID3D12Device> device,
		ComPtr<ID3D12GraphicsCommandList> commandList, UINT width, UINT height);
	void Update(float dt);
	void Compute(ComPtr<ID3D12GraphicsCommandList>& commandList);
	void Render(ComPtr<ID3D12GraphicsCommandList>& commandList,
		ComPtr<ID3D12Resource>& globalConstsUploadHeap);

	SimParams m_constantBufferData;
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