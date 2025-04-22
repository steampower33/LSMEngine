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

#define STRUCTURED_CNT 8
#define CONSTANT_CNT 1

class SphSimulator
{
public:
	SphSimulator();
	~SphSimulator();

	// ���� ����
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
		UINT particleID = 0; // ���� ��ƼŬ �ε���
		int cellIndex = 0;  // ���� �ؽ� ��
		UINT flag = 0; // �׷� ���� �÷���
	};

	struct CompactCell
	{
		int cellIndex = 0;
		UINT startIndex = 0;
		UINT endIndex = 0;
	};

	// �ùķ��̼� �Ķ���� (��� ���ۿ�)
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
		float mass = 1.0f;
		float pressureCoeff = 0.1f;
		float density0 = 1.0f;

		float viscosity = 0.1f;
		float gravityCoeff;
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
	const UINT m_nX = 16;
	const UINT m_nY = 16;
	const UINT m_nZ = 16;
	const UINT m_numParticles = m_nX * m_nY * m_nZ;
	const float m_radius = 0.2f;
	const float m_dp = 0.2f;
	float m_smoothingRadius = 0.5f;
	float m_maxBoundsX = 4.0f;
	float m_maxBoundsY = 4.0f;
	float m_maxBoundsZ = 4.0f;
	float m_gravityCoeff = 1.0f;
	float m_collisionDamping = 0.95f;
	UINT m_gridDimX = static_cast<UINT>(ceilf(m_maxBoundsX * 2.0f / m_smoothingRadius));
	UINT m_gridDimY = static_cast<UINT>(ceilf(m_maxBoundsY * 2.0f / m_smoothingRadius));
	UINT m_gridDimZ = static_cast<UINT>(ceilf(m_maxBoundsZ * 2.0f / m_smoothingRadius));
	UINT m_cellCnt = m_gridDimX * m_gridDimY * m_gridDimZ;
	
private:
	const UINT m_particleDataSize = sizeof(Particle);
	const UINT m_particleHashDataSize = sizeof(ParticleHash);
	const UINT m_compactCellDataSize = sizeof(CompactCell);
	const UINT m_compactCellDataCnt = m_cellCnt;

	vector<Particle> m_particles;
	vector<XMFLOAT3> m_particlePositions;

	ComPtr<ID3D12DescriptorHeap> m_cbvSrvUavHeap;
	ComPtr<ID3D12DescriptorHeap> m_clearHeap;
	UINT m_cbvSrvUavSize = 0;

	// Ping Pong �������� ���� ����(Particle Structured)
	// �� ������ ���� ���� �д� ���۸� ����
	// ���۴� srv, uav �ϳ���
	// �ʱ� ����
	// SRV | UAV | UAV
	ComPtr<ID3D12Resource> m_structuredBuffer[STRUCTURED_CNT];
	ComPtr<ID3D12Resource> m_particlesUploadBuffer;
	ComPtr<ID3D12Resource> m_particlePositionsUploadBuffer;
	ComPtr<ID3D12Resource> m_particlesHashUploadBuffer;
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_structuredBufferSrvGpuHandle[STRUCTURED_CNT];
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_structuredBufferUavGpuHandle[STRUCTURED_CNT];

	ComPtr<ID3D12Resource> m_constantBuffer;
	UINT8* m_constantBufferDataBegin = nullptr;
	UINT m_constantBufferSize = (sizeof(SimParams) + 255) & ~255;
	D3D12_CPU_DESCRIPTOR_HANDLE m_constantBufferCbvCpuHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_constantBufferCbvGpuHandle;

	UINT m_particleAIndex = 1;
	UINT m_particleBIndex = 0;
	UINT m_particlePositionIndex = 2;
	UINT m_cellCountIndex = 3;
	UINT m_cellOffsetIndex = 4;
	UINT m_cellStartIndex = 5;
	UINT m_cellStartPartialSumIndex = 6;
	UINT m_cellScatterIndex = 7;

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