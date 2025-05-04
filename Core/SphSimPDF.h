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

#define STRUCTURED_CNT 7
#define CONSTANT_CNT 2

class SphSimPDF
{
public:
	SphSimPDF();
	~SphSimPDF();

	// 입자 구조
	struct Particle {
		XMFLOAT3 position = XMFLOAT3(0.0f, 0.0f, 0.0f);
		XMFLOAT3 velocity = XMFLOAT3(0.0f, 0.0f, 0.0f);
		XMFLOAT3 predictedPosition = XMFLOAT3(0.0f, 0.0f, 0.0f);
		float density = 0.0f;
		float nearDensity = 0.0f;
		float spawnTime = -1.0f;
		float p;
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

	// Simulator Param
	__declspec(align(256)) struct SimParams {
		float deltaTime = 1 / 120.0f;
		UINT numParticles;
		float smoothingRadius;
		UINT cellCnt;

		XMFLOAT3 minBounds;
		float currentTime = 0.0f;
		XMFLOAT3 maxBounds;
		float endTime = 0.0f;

		int gridDimX;
		int gridDimY;
		int gridDimZ;
		UINT forceKey = 0;

		float density0 = 1000.0f;
		float pressureCoeff = 30.0f;
		float nearPressureCoeff = 2.0f;
		float viscosity = 0.4f;

		float mass = 0.3f;
		float radius = 0.0f;
		float boundaryStiffness = 1000.0f;
		float boundaryDamping = 1.5f;

		float gravityCoeff = 1.0f;
		float duration = 1.0f;
		float startTime;
		float p3;
	};

	SimParams m_simParamsData;
	const UINT m_groupSizeX = 512;
	float m_smoothingRadius = 0.1f;
	const float m_radius = m_smoothingRadius * 0.5f;
	const float m_dp = m_smoothingRadius * 0.8f;
	float m_maxBoundsX = 2.0f;
	float m_maxBoundsY = 2.0f;
	float m_maxBoundsZ = 2.0f;

	UINT m_gridDimX = static_cast<UINT>(ceil(m_maxBoundsX * 2.0f / m_smoothingRadius));
	UINT m_gridDimY = static_cast<UINT>(ceil(m_maxBoundsY * 2.0f / m_smoothingRadius));
	UINT m_gridDimZ = static_cast<UINT>(ceil(m_maxBoundsZ * 2.0f / m_smoothingRadius));
	const UINT m_nX = 32;
	const UINT m_nY = 32;
	const UINT m_nZ = 32;
	const UINT m_numParticles = m_nX * m_nY * m_nZ * 2;
	UINT m_cellCnt = m_numParticles;

	void Initialize(ComPtr<ID3D12Device> device,
		ComPtr<ID3D12GraphicsCommandList> commandList, UINT width, UINT height);

	void UpdatePDF(float dt, UINT& forceKey);
	void ComputePDF(ComPtr<ID3D12GraphicsCommandList>& commandList, bool& reset);
	void RenderPDF(ComPtr<ID3D12GraphicsCommandList>& commandList,
		ComPtr<ID3D12Resource>& globalConstsUploadHeap);
private:
	const UINT m_particleDataSize = sizeof(Particle);
	const UINT m_particleHashDataSize = sizeof(ParticleHash);
	const UINT m_compactCellDataSize = sizeof(CompactCell);
	const UINT m_compactCellDataCnt = m_cellCnt;

	vector<Particle> m_particles;

	ComPtr<ID3D12DescriptorHeap> m_cbvSrvUavHeap;
	ComPtr<ID3D12DescriptorHeap> m_clearHeap;
	UINT m_cbvSrvUavSize = 0;

	ComPtr<ID3D12Resource> m_structuredBuffer[STRUCTURED_CNT];
	ComPtr<ID3D12Resource> m_particlesUploadBuffer;
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_structuredBufferSrvGpuHandle[STRUCTURED_CNT];
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_structuredBufferUavGpuHandle[STRUCTURED_CNT];

	ComPtr<ID3D12Resource> m_simParamsConstantBuffer;
	UINT8* m_simParamsConstantBufferDataBegin = nullptr;
	UINT m_simParamsConstantBufferSize = (sizeof(SimParams) + 255) & ~255;
	D3D12_CPU_DESCRIPTOR_HANDLE m_simParamsCbvCpuHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_simParamsCbvGpuHandle;

	UINT m_particleAIndex = 1;
	UINT m_particleBIndex = 0;
	UINT m_cellCountIndex = 2;
	UINT m_cellOffsetIndex = 3;
	UINT m_cellStartIndex = 4;
	UINT m_cellStartPartialSumIndex = 5;
	UINT m_cellScatterIndex = 6;

	void CreateConstantBuffer(ComPtr<ID3D12Device> device);
	void GenerateEmitterParticles();
	void GenerateDamParticles();
	void CreateStructuredBufferWithViews(
		ComPtr<ID3D12Device>& device, UINT bufferIndex, UINT srvIndex, UINT uavIndex, UINT dataSize, UINT dataCnt, wstring dataName);
	void UploadAndCopyData(ComPtr<ID3D12Device> device,
		ComPtr<ID3D12GraphicsCommandList> commandList, UINT dataSize, ComPtr<ID3D12Resource>& uploadBuffer, wstring dataName, ComPtr<ID3D12Resource>& destBuffer, D3D12_RESOURCE_STATES destBufferState);
};