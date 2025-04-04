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

	// 입자 구조
	struct Particle {
		XMFLOAT2 position = XMFLOAT2(0.0f, 0.0f);
		XMFLOAT2 velocity = XMFLOAT2(0.0f, 0.0f);

		XMFLOAT4 color = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);

		float life = 0.0f;
		float size = 1.0f;
		float padding1;
		float padding2;
	};

	// 시뮬레이션 파라미터 (상수 버퍼용)
	__declspec(align(256)) struct SimParams {
		float deltaTime = 0.0f;
		XMFLOAT2 gravity = XMFLOAT2(0.0f, -9.8f); // 중력
		UINT numParticles = 128;

		XMFLOAT2 minBounds = XMFLOAT2(-5.0f, -5.0f); // 경계 최소값
		XMFLOAT2 maxBounds = XMFLOAT2(5.0f, 5.0f);   // 경계 최대값
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

	// Ping Pong 형식으로 버퍼 구성(Particle Structured)
	// 매 프레임 마다 쓰고 읽는 버퍼를 변경
	// 버퍼당 srv, uav 하나씩
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

	// 초기에 Update함수를 먼저 거치기에 read는 0번, write는 1번으로 시작 할수 있도록 초기에 설정
	UINT m_readIdx = 1;
	UINT m_writeIdx = 0;

	void GenerateParticles();
	void CreateStructuredBuffer(ComPtr<ID3D12Device>& device, UINT width, UINT height, UINT index);
	void UploadAndCopyParticleData(ComPtr<ID3D12Device> device,
		ComPtr<ID3D12GraphicsCommandList> commandList);
};