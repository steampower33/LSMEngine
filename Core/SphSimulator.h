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

#define STRUCTURED_CNT 4
#define CONSTANT_CNT 1

class SphSimulator
{
public:
	SphSimulator();
	~SphSimulator();

	// 입자 구조
	struct Particle {
		XMFLOAT3 position = XMFLOAT3(0.0f, 0.0f, 0.0f);
		float radius = 0.0f;
		XMFLOAT3 velocity = XMFLOAT3(0.0f, 0.0f, 0.0f);
		float life = -1.0f;
		XMFLOAT3 color = XMFLOAT3(1.0f, 1.0f, 1.0f);
		float density = 0.0f;
		XMFLOAT3 force = XMFLOAT3(0.0f, 0.0f, 0.0f);
		float pressure = 0.0f;
	};

	struct ParticleHash
	{
		UINT particleID = 0; // 원래 파티클 인덱스
		int cellIndex = 0;  // 계산된 해시 값
		UINT flag = 0; // 그룹 시작 플래그
	};

	struct CompactCell
	{
		int cellIndex = 0;
		UINT startIndex = 0;
		UINT endIndex = 0;
	};

	// 시뮬레이션 파라미터 (상수 버퍼용)
	__declspec(align(256)) struct SimParams {
		float deltaTime;
		UINT numParticles;
		float smoothingRadius;
		UINT cellCnt;

		XMFLOAT3 minBounds;
		int gridDimX;
		XMFLOAT3 maxBounds;
		int gridDimY;

		int gridDimZ;
		UINT maxParticles;
		float mass = 1.0f;
		float pressureCoeff = 50.0f;

		float density0 = 10.0f;
		float viscosity = 0.7f;
		float gravity;
		float collisionDamping;

		UINT forceKey;
	};

	void Initialize(ComPtr<ID3D12Device> device,
		ComPtr<ID3D12GraphicsCommandList> commandList, UINT width, UINT height);
	void Update(float dt, UINT forceKey);
	void Compute(ComPtr<ID3D12GraphicsCommandList>& commandList);
	void Render(ComPtr<ID3D12GraphicsCommandList>& commandList,
		ComPtr<ID3D12Resource>& globalConstsUploadHeap);

	SimParams m_constantBufferData;
	const UINT m_groupSizeX = 512;
	const UINT m_nX = 64;
	const UINT m_nY = 64;
	const UINT m_nZ = 8;
	const UINT m_maxParticles = m_nX * m_nY * m_nZ;
	const float m_radius = 0.2f;
	const float m_dp = 0.2f;
	float m_smoothingRadius = 0.7f;
	float m_maxBoundsX = 20.0f;
	float m_maxBoundsY = 15.0f;
	float m_maxBoundsZ = 5.0f;
	float m_gravityCoeff = 1.0f;
	float m_collisionDamping = 0.95f;
	float m_gridDimX = static_cast<UINT>(m_maxBoundsX * 2.0f / m_smoothingRadius);
	float m_gridDimY = static_cast<UINT>(m_maxBoundsY * 2.0f / m_smoothingRadius);
	float m_gridDimZ = static_cast<UINT>(m_maxBoundsZ * 2.0f / m_smoothingRadius);
	UINT m_cellCnt = m_maxParticles * 2;
	
private:
	const UINT m_particleDataSize = sizeof(Particle);
	const UINT m_particleHashDataSize = sizeof(ParticleHash);
	const UINT m_compactCellDataSize = sizeof(CompactCell);
	const UINT m_compactCellDataCnt = m_cellCnt;

	vector<Particle> m_particles;

	// Ping-Pong       | SimParams | Hash    |
	// SRV UAV SRV UAV | CBV       | SRV UAV |
	ComPtr<ID3D12DescriptorHeap> m_cbvSrvUavHeap;
	UINT m_cbvSrvUavSize = 0;

	// Ping Pong 형식으로 버퍼 구성(Particle Structured)
	// 매 프레임 마다 쓰고 읽는 버퍼를 변경
	// 버퍼당 srv, uav 하나씩
	// 초기 상태
	// SRV | UAV | UAV
	ComPtr<ID3D12Resource> m_structuredBuffer[STRUCTURED_CNT];
	ComPtr<ID3D12Resource> m_particlesUploadBuffer;
	ComPtr<ID3D12Resource> m_particlesHashUploadBuffer;
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_particleBufferSrvGpuHandle[STRUCTURED_CNT];
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_particleBufferUavGpuHandle[STRUCTURED_CNT];

	ComPtr<ID3D12Resource> m_constantBuffer;
	UINT8* m_constantBufferDataBegin = nullptr;
	UINT m_constantBufferSize = (sizeof(SimParams) + 255) & ~255;
	D3D12_CPU_DESCRIPTOR_HANDLE m_constantBufferCbvCpuHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_constantBufferCbvGpuHandle;

	UINT m_particleA = 1;
	UINT m_particleB = 0;
	UINT m_hashIdx = 2;
	UINT m_compactCellIdx = 3;

	void GenerateParticles();
	void CreateStructuredBufferWithViews(
		ComPtr<ID3D12Device>& device, UINT bufferIndex, UINT srvIndex, UINT uavIndex, UINT dataSize, UINT dataCnt, wstring dataName);
	void UploadAndCopyData(ComPtr<ID3D12Device> device,
		ComPtr<ID3D12GraphicsCommandList> commandList, UINT dataSize, ComPtr<ID3D12Resource>& uploadBuffer, wstring dataName, ComPtr<ID3D12Resource>& destBuffer, D3D12_RESOURCE_STATES destBufferState);

	void CalcHashes(ComPtr<ID3D12GraphicsCommandList> commandList);
	void BitonicSort(ComPtr<ID3D12GraphicsCommandList> commandList);
	void FlagGeneration(ComPtr<ID3D12GraphicsCommandList> commandList);
	void ScatterCompactCell(ComPtr<ID3D12GraphicsCommandList> commandList);
	void CalcDensityForces(ComPtr<ID3D12GraphicsCommandList> commandList);
	void CalcSPH(ComPtr<ID3D12GraphicsCommandList> commandList);

	// Unuse
	void FlagScan(ComPtr<ID3D12GraphicsCommandList> commandList);
};