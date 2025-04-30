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

class SphSimulator
{
public:
	SphSimulator();
	~SphSimulator();

	// ���� ����
	struct Particle {
		XMFLOAT3 position = XMFLOAT3(0.0f, 0.0f, 0.0f);
		XMFLOAT3 velocity = XMFLOAT3(0.0f, 0.0f, 0.0f);
		XMFLOAT3 predictedPosition = XMFLOAT3(0.0f, 0.0f, 0.0f);
		float density = 0.0f;
		float nearDensity = 0.0f;
		UINT isGhost = false;
		float spawnTime = -1.0f;
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

	// Simulator Param
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
		float radius = 0.1f;
		float currentTime = 0.0f;
		
		float pressureCoeff = 40.0f;
		float nearPressureCoeff = 30.0f;
		float density0 = 100.0f;
		float viscosity = 0.1f;
		
		float gravityCoeff = 1.0f;
		float collisionDamping = 0.0f;
		UINT forceKey = 0;
		float p1;
	};

	void Initialize(ComPtr<ID3D12Device> device,
		ComPtr<ID3D12GraphicsCommandList> commandList, UINT width, UINT height);
	void Update(float dt, UINT& forceKey);
	void Compute(ComPtr<ID3D12GraphicsCommandList>& commandList);
	void Render(ComPtr<ID3D12GraphicsCommandList>& commandList,
		ComPtr<ID3D12Resource>& globalConstsUploadHeap);

	SimParams m_simParamsData;
	const float m_deltaTime = 1 / 120.0f;
	const UINT m_groupSizeX = 512;
	float m_smoothingRadius = 0.2f;
	const float m_radius = m_smoothingRadius * 0.5f;
	const float m_dp = m_smoothingRadius;
	float m_maxBoundsX = 10.0f;
	float m_minBoundsMoveX = -m_maxBoundsX;
	float m_maxBoundsY = 5.0f;
	float m_maxBoundsZ = 4.0f;
	float m_collisionDamping = 0.2f;

	UINT m_gridDimX = static_cast<UINT>(ceil(m_maxBoundsX * 2.0f / m_smoothingRadius));
	UINT m_gridDimY = static_cast<UINT>(ceil(m_maxBoundsY * 2.0f / m_smoothingRadius));
	UINT m_gridDimZ = static_cast<UINT>(ceil(m_maxBoundsZ * 2.0f / m_smoothingRadius));
	UINT m_wallXCnt = m_maxBoundsX * 2 / m_dp + 1;
	UINT m_wallYCnt = m_maxBoundsY * 2 / m_dp + 1;
	UINT m_wallZCnt = m_maxBoundsZ * 2 / m_dp + 1;
	UINT m_ghostCnt =
		m_wallXCnt * m_wallZCnt * 2 +
		m_wallXCnt * m_wallYCnt * 2 +
		m_wallYCnt * m_wallZCnt * 2;
	const UINT m_nX = 80;
	const UINT m_nY = 30;
	const UINT m_nZ = 20;
	//const UINT m_numParticles = m_nX * m_nY * m_nZ + m_ghostCnt;
	const UINT m_numParticles = m_nX * m_nY * m_nZ;
	UINT m_cellCnt = m_numParticles * 2;
	
private:
	const UINT m_particleDataSize = sizeof(Particle);
	const UINT m_particleHashDataSize = sizeof(ParticleHash);
	const UINT m_compactCellDataSize = sizeof(CompactCell);
	const UINT m_compactCellDataCnt = m_cellCnt;

	vector<Particle> m_particles;

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
	void GenerateGhostParticles();
	void GenerateEmitterParticles();
	void GenerateDamParticles();
	void CreateStructuredBufferWithViews(
		ComPtr<ID3D12Device>& device, UINT bufferIndex, UINT srvIndex, UINT uavIndex, UINT dataSize, UINT dataCnt, wstring dataName);
	void UploadAndCopyData(ComPtr<ID3D12Device> device,
		ComPtr<ID3D12GraphicsCommandList> commandList, UINT dataSize, ComPtr<ID3D12Resource>& uploadBuffer, wstring dataName, ComPtr<ID3D12Resource>& destBuffer, D3D12_RESOURCE_STATES destBufferState);

	void CalcDensityForces(ComPtr<ID3D12GraphicsCommandList> commandList);
	void CalcSPH(ComPtr<ID3D12GraphicsCommandList> commandList);

};