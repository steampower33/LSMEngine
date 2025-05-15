#pragma once

#include <iostream>

#include <d3d12.h>
#include "d3dx12.h"
#include <dxgi1_6.h>
#include <DirectXMath.h>

#include "Helpers.h"
#include "ConstantBuffers.h"
#include "GraphicsCommon.h"
#include "Camera.h"
#include "Mesh.h"
#include "MeshData.h"
#include "Vertex.h"
#include "Model.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;
using namespace std;

#define STRUCTURED_CNT 12
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
		float pressureCoeff = 80.0f;
		float nearPressureCoeff = 2.0f;
		float viscosity = 0.1f;
	
		float mass = 1.0f;
		float radius = 0.0f;
		float boundaryStiffness = 1000.0f;
		float boundaryDamping = 1.0f;
		
		float gravityCoeff = 1.0f;
		float duration = 1.0f;
		float startTime;
		UINT reset;

		UINT nX;
		UINT nY;
		UINT nZ;
		float dp;

		XMFLOAT4X4 p3;
		XMFLOAT4X4 p4;
	};

	// Render Param
	__declspec(align(256)) struct RenderParams {
		XMFLOAT4X4 view;
		XMFLOAT4X4 proj;

		float thicknessContributionScale = 0.01f;
		float p1;
		float p2;
		float p3;

		XMFLOAT4 p4;
		XMFLOAT4 p5;
		XMFLOAT4 p6;
		
		XMFLOAT4X4 p7;
	};
	
	// Compute Param
	__declspec(align(256)) struct ComputeParams {
		XMFLOAT4X4 invProj;
		XMFLOAT4X4 invView;

		int filterRadius = 16;
		float sigmaSpatial = 8.0f;
		float sigmaDepth = 1.0f;
		float opacityMultiplier = 1000.0f;

		UINT width;
		float invWidth;
		UINT height;
		float invHeight;

		XMFLOAT3 eyeWorld;
		float refractionStrength = 0.1f;

		XMFLOAT3 specularColor = XMFLOAT3(1.0f, 1.0f, 1.0f);
		float waterDensity = 0.1f;

		float fresnelClamp = 0.2f;
		float p0;
		float p1;
		float p2;

		XMFLOAT4 p3;
		XMFLOAT4 p4;
		XMFLOAT4 p5;
	};

	SimParams m_simParamsData;
	RenderParams m_renderParamsData;
	ComputeParams m_computeParamsData;
	const UINT m_groupSizeX = 512;
	float m_smoothingRadius = 0.18f;
	float m_radius = m_smoothingRadius * 0.5f;
	float m_dp = m_smoothingRadius * 0.5f;
	float m_maxBoundsX = 4.0f;
	float m_maxBoundsY = 4.0f;
	float m_maxBoundsZ = 4.0f;

	XMFLOAT3 m_emitterPos = XMFLOAT3{ 0.0f, 1.0f, 0.0f };
	XMFLOAT3 m_emitterDir = XMFLOAT3{ 1.0f, -1.0f, 0.0f };
	float m_emitterVel = 5.0f;
	float m_spawnTimeStep = 2.0f;

	UINT m_gridDimX = static_cast<UINT>(ceil(m_maxBoundsX * 2.0f / m_smoothingRadius));
	UINT m_gridDimY = static_cast<UINT>(ceil(m_maxBoundsY * 2.0f / m_smoothingRadius));
	UINT m_gridDimZ = static_cast<UINT>(ceil(m_maxBoundsZ * 2.0f / m_smoothingRadius));
	const UINT m_nX = 50;
	const UINT m_nY = 20;
	const UINT m_nZ = 50;
	const UINT m_numParticles = m_nX * m_nY * m_nZ * 2;
	UINT m_cellCnt = m_numParticles < 0 ? 2048 : m_numParticles;

	void Initialize(ComPtr<ID3D12Device> device, ComPtr<ID3D12GraphicsCommandList> commandList,
		ComPtr<ID3D12CommandQueue> commandQueue, UINT width, UINT height);

	void Update(float dt, UINT& forceKey, UINT& reset, shared_ptr<Camera>& camera, bool isPaused, shared_ptr<Model>& skybox);
	void Compute(ComPtr<ID3D12GraphicsCommandList>& commandList, UINT& reset);
	void Render(ComPtr<ID3D12GraphicsCommandList>& commandList,
		ComPtr<ID3D12Resource>& globalConstsUploadHeap);

	void InitializeDesciptorHeaps(ComPtr<ID3D12Device>& device, UINT width, UINT height);

	UINT m_renderSRVCnt = 8;
	UINT m_renderCubemapSRVCnt = 2;
	UINT m_renderUAVCnt = 5;
	UINT m_renderCBVCnt = 2;
	UINT m_renderHeapCnt = m_renderSRVCnt + m_renderCubemapSRVCnt + m_renderUAVCnt + m_renderCBVCnt + Graphics::imguiTextureSize;
	UINT m_renderUAVStart = m_renderSRVCnt + m_renderCubemapSRVCnt;

	ComPtr<ID3D12DescriptorHeap> m_renderHeap;

	ComPtr<ID3D12Resource> m_particleRTVBuffer;
	ComPtr<ID3D12DescriptorHeap> m_particleRTVHeap;
	UINT m_particleRenderSRVIndex = 0;
	ComPtr<ID3D12Resource> m_particleDSVBuffer;
	ComPtr<ID3D12DescriptorHeap> m_particleDSVHeap;

	ComPtr<ID3D12Resource> m_thicknessRTVBuffer;
	ComPtr<ID3D12DescriptorHeap> m_thicknessRTVHeap;
	UINT m_thicknessSRVIndex = 1;
	UINT m_thicknessUAVIndex = m_renderUAVStart;

	ComPtr<ID3D12Resource> m_particleDepthOutputBuffer;
	ComPtr<ID3D12DescriptorHeap> m_particleDepthOutputRTVHeap;
	UINT m_particleDepthOutputSRVIndex = 2;

	ComPtr<ID3D12Resource> m_smoothedDepthBuffer;
	UINT m_smoothedDepthSRVIndex = 3;
	UINT m_smoothedDepthUAVIndex = m_renderUAVStart + 1;

	ComPtr<ID3D12Resource> m_normalBuffer;
	UINT m_normalSRVIndex = 4;
	UINT m_normalUAVIndex = m_renderUAVStart + 2;

	ComPtr<ID3D12Resource> m_sceneRTVBuffer;
	ComPtr<ID3D12DescriptorHeap> m_sceneRTVHeap;
	UINT m_sceneSRVIndex = 5;
	UINT m_sceneUAVIndex = m_renderUAVStart + 3;

	ComPtr<ID3D12Resource> m_backgroundRTVBuffer;
	ComPtr<ID3D12DescriptorHeap> m_backgroundRTVHeap;
	UINT m_backgroundSRVIndex = 6;
	ComPtr<ID3D12Resource> m_backgroundDSVBuffer;
	ComPtr<ID3D12DescriptorHeap> m_backgroundDSVHeap;

	ComPtr<ID3D12Resource> m_shadedRTVBuffer;
	UINT m_shadedSRVIndex = 7;
	UINT m_shadedUAVIndex = m_renderUAVStart + 4;

	UINT m_renderCBVIndex = m_renderUAVStart + m_renderUAVCnt;

	UINT m_cubemapSRVIndex = m_renderSRVCnt;

	UINT m_finalSRVIndex = m_sceneSRVIndex;

	bool m_backgroundRender = false;
	bool m_particleRender = false;
	bool m_particleDepthRender = false;
	bool m_smoothingDepthRender = false;
	bool m_normalRender = false;
	bool m_thicknessRender = false;
	bool m_shadedRender = false;
	bool m_sceneRender = true;

	ComPtr<ID3D12Resource> diffuseBuffer;
	ComPtr<ID3D12Resource> specularBuffer;

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

	ComPtr<ID3D12Resource> m_renderParamsConstantBuffer;
	UINT8* m_renderParamsConstantBufferDataBegin = nullptr;
	UINT m_renderParamsConstantBufferSize = (sizeof(RenderParams) + 255) & ~255;

	ComPtr<ID3D12Resource> m_computeParamsConstantBuffer;
	UINT8* m_computeParamsConstantBufferDataBegin = nullptr;
	UINT m_computeParamsConstantBufferSize = (sizeof(ComputeParams) + 255) & ~255;

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
	void CreateCubeMapTexture(
		ComPtr<ID3D12Device>& device,
		ComPtr<ID3D12CommandQueue>& commandQueue,
		ComPtr<ID3D12Resource>& buffer,
		const string& filepath,
		UINT srvIndex,
		bool isCubeMap);

	shared_ptr<Mesh> dummy;
};