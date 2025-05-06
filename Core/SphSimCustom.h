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

#define STRUCTURED_CNT 13
#define CONSTANT_CNT 1

class SphSimCustom
{
public:
	SphSimCustom();
	~SphSimCustom();

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
		float pressureCoeff = 40.0f;
		float nearPressureCoeff = 15.0f;
		float viscosity = 0.1f;
	
		float mass = 0.7f;
		float radius = 0.0f;
		float boundaryStiffness = 1000.0f;
		float boundaryDamping = 1.4f;
		
		float gravityCoeff = 1.0f;
		float duration = 1.0f;
		float startTime;
		float p3;
	};

	SimParams m_simParamsData;
	const UINT m_groupSizeX = 512;
	float m_smoothingRadius = 0.15f;
	float m_radius = m_smoothingRadius * 0.5f;
	float m_dp = m_smoothingRadius * 0.5f;
	float m_maxBoundsX = 4.0f;
	float m_maxBoundsY = 4.0f;
	float m_maxBoundsZ = 4.0f;

	UINT m_gridDimX = static_cast<UINT>(ceil(m_maxBoundsX * 2.0f / m_smoothingRadius));
	UINT m_gridDimY = static_cast<UINT>(ceil(m_maxBoundsY * 2.0f / m_smoothingRadius));
	UINT m_gridDimZ = static_cast<UINT>(ceil(m_maxBoundsZ * 2.0f / m_smoothingRadius));
	const UINT m_nX = 32;
	const UINT m_nY = 32;
	const UINT m_nZ = 32;
	const UINT m_numParticles = m_nX * m_nY * m_nZ * 2;
	UINT m_cellCnt = m_numParticles < 0 ? 2048 : m_numParticles;

	void Initialize(ComPtr<ID3D12Device> device,
		ComPtr<ID3D12GraphicsCommandList> commandList, UINT width, UINT height);

	void Update(float dt, UINT& forceKey, UINT& reset);
	void Compute(ComPtr<ID3D12GraphicsCommandList>& commandList, UINT& reset);
	void Render(ComPtr<ID3D12GraphicsCommandList>& commandList,
		ComPtr<ID3D12Resource>& globalConstsUploadHeap);

	void InitializeDesciptorHeaps(ComPtr<ID3D12Device>& device, float width, float height);

	UINT m_renderSRVCnt = 3;
	UINT m_rednerUAVCnt = 1;
	UINT m_renderHeapCnt = m_renderSRVCnt + m_rednerUAVCnt + Graphics::imguiTextureSize;
	
	ComPtr<ID3D12DescriptorHeap> m_renderHeap;

	ComPtr<ID3D12Resource> m_particleRTVBuffer;
	ComPtr<ID3D12DescriptorHeap> m_particleRTVHeap;
	UINT m_fluidRenderSRVIndex = 0;
	ComPtr<ID3D12Resource> m_particleDepthOutputTexture;
	ComPtr<ID3D12DescriptorHeap> m_particleDepthOutputRTVHeap;
	UINT m_particleDepthOutputSRVIndex = 1;
	ComPtr<ID3D12Resource> m_particleDSVBuffer;
	ComPtr<ID3D12DescriptorHeap> m_fluidDSVHeap;
	UINT m_particleDepthSRVIndex = 2;
	
	ComPtr<ID3D12Resource> m_sceneRTVBuffer;
	ComPtr<ID3D12DescriptorHeap> m_sceneRTVHeap;
	UINT m_sceneSRVIndex = 3;
	UINT m_sceneUAVIndex = m_renderSRVCnt;

private:
	const UINT m_particleHashDataSize = sizeof(ParticleHash);
	const UINT m_compactCellDataSize = sizeof(CompactCell);
	const UINT m_compactCellDataCnt = m_cellCnt;

	vector<XMFLOAT3> m_position;
	vector<XMFLOAT3> m_velocity;
	vector<float> m_spawnTime;

	ComPtr<ID3D12DescriptorHeap> m_cbvSrvUavHeap;
	ComPtr<ID3D12DescriptorHeap> m_clearHeap;
	UINT m_rtvSize = 0;
	UINT m_cbvSrvUavSize = 0;
	UINT m_dsvSize = 0;

	ComPtr<ID3D12Resource> m_structuredBuffer[STRUCTURED_CNT];
	ComPtr<ID3D12Resource> m_positionUploadBuffer;
	ComPtr<ID3D12Resource> m_velocityUploadBuffer;
	ComPtr<ID3D12Resource> m_spawnTimeUploadBuffer;
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_structuredBufferSrvCpuHandle[STRUCTURED_CNT];
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_structuredBufferSrvGpuHandle[STRUCTURED_CNT];
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_structuredBufferUavCpuHandle[STRUCTURED_CNT];
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_structuredBufferUavGpuHandle[STRUCTURED_CNT];

	ComPtr<ID3D12Resource> m_simParamsConstantBuffer;
	UINT8* m_simParamsConstantBufferDataBegin = nullptr;
	UINT m_simParamsConstantBufferSize = (sizeof(SimParams) + 255) & ~255;
	D3D12_CPU_DESCRIPTOR_HANDLE m_simParamsCbvCpuHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_simParamsCbvGpuHandle;

	UINT m_positionIndex = 0;
	UINT m_predictedPositionIndex = 1;
	UINT m_velocityIndex = 2;
	UINT m_predictedVelocityIndex = 3;
	UINT m_densityIndex = 4;
	UINT m_nearDensityIndex = 5;

	UINT m_spawnTimeIndex = 6;

	UINT m_cellCountIndex = 7;
	UINT m_cellOffsetIndex = 8;
	UINT m_cellStartIndex = 9;
	UINT m_cellStartPartialSumIndex = 10;	
	UINT m_cellScatterIndex = 11;

	UINT m_colorIndex = 12;

	UINT m_width;
	UINT m_height;

	void CreateConstantBuffer(ComPtr<ID3D12Device> device);
	void GenerateEmitterParticles();
	void GenerateDamParticles();
	void CreateStructuredBufferWithViews(
		ComPtr<ID3D12Device>& device, UINT bufferIndex, UINT dataSize, UINT dataCnt, wstring dataName);

	template <typename T>
	void UploadAndCopyData(ComPtr<ID3D12Device> device,
		ComPtr<ID3D12GraphicsCommandList> commandList, vector<T>& data, UINT dataSize, ComPtr<ID3D12Resource>& uploadBuffer, wstring dataName, ComPtr<ID3D12Resource>& destBuffer, D3D12_RESOURCE_STATES destBufferState);
};