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

#define STRUCTURED_CNT 7
#define CONSTANT_CNT 1

class SphSimulator
{
public:
	SphSimulator();
	~SphSimulator();

	// 입자 구조
	struct Particle {
		XMFLOAT3 position = XMFLOAT3(0.0f, 0.0f, 0.0f);
		float p1;
		XMFLOAT3 velocity = XMFLOAT3(0.0f, 0.0f, 0.0f);
		float p2;
		XMFLOAT3 color = XMFLOAT3(1.0f, 1.0f, 1.0f);
		float p3;
		float size = 0.0f;
		float life = -1.0f;
		float p4;
		float p5;
	};

	struct ParticleHash
	{
		UINT particleID = 0; // 원래 파티클 인덱스
		UINT hashValue = 0;  // 계산된 해시 값
		UINT flag = 0; // 그룹 시작 플래그
	};

	struct ScanResult
	{
		UINT groupID = 0;
	};

	struct CompactCell
	{
		UINT hashValue = 0;
		UINT startIndex = 0;
		UINT endIndex = 0;
	};

	// 시뮬레이션 파라미터 (상수 버퍼용)
	__declspec(align(256)) struct SimParams {
		float deltaTime = 0.0f;
		XMFLOAT2 gravity = XMFLOAT2(0.0f, -9.8f); // 중력
		UINT numParticles;

		XMFLOAT3 minBounds;
		float gridDimX;
		XMFLOAT3 maxBounds;
		float gridDimY;

		float gridDimZ;
		float cellSize;
		float p2;
		float p3;
	};

	float m_minBounds[3] = { -4.0f, -4.0f, 0.0f };
	float m_maxBounds[3] = { 4.0f, 4.0f, 0.0f };

	void Initialize(ComPtr<ID3D12Device> device,
		ComPtr<ID3D12GraphicsCommandList> commandList, UINT width, UINT height);
	void Update(float dt);
	void Compute(ComPtr<ID3D12GraphicsCommandList>& commandList);
	void Render(ComPtr<ID3D12GraphicsCommandList>& commandList,
		ComPtr<ID3D12Resource>& globalConstsUploadHeap);

	SimParams m_constantBufferData;
private:
	const UINT m_maxParticles = 1024;
	const UINT m_groupSizeX = 512;
	const float m_radius = 1.0f / 16.0f;
	const float m_cellSize = m_radius * 4.0f;
	float m_gridDimX = m_maxBounds[0] - m_minBounds[0];
	float m_gridDimY = m_maxBounds[1] - m_minBounds[1];
	float m_gridDimZ = m_maxBounds[2] - m_minBounds[2];
	UINT m_cellCnt = static_cast<UINT>(m_gridDimX / m_cellSize) * static_cast<UINT>(m_gridDimY / m_cellSize);

	const UINT m_particleDataSize = sizeof(Particle);
	const UINT m_particleHashDataSize = sizeof(ParticleHash);
	const UINT m_scanResultDataSize = sizeof(ScanResult);
	const UINT m_scanResultDataCnt = static_cast<UINT>((m_maxParticles) / m_groupSizeX);
	const UINT m_compactCellDataSize = sizeof(CompactCell);
	const UINT m_compactCellDataCnt = m_cellCnt;

	vector<Particle> m_particles;
	vector<ParticleHash> m_particlesHashes;
	vector<ScanResult> m_localScan;
	vector<ScanResult> m_partialSums;

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
	ComPtr<ID3D12Resource> m_scanResultsUploadBuffer;
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_particleBufferSrvGpuHandle[STRUCTURED_CNT];
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_particleBufferUavGpuHandle[STRUCTURED_CNT];

	ComPtr<ID3D12Resource> m_constantBuffer;
	UINT8* m_constantBufferDataBegin = nullptr;
	UINT m_constantBufferSize = (sizeof(SimParams) + 255) & ~255;
	D3D12_CPU_DESCRIPTOR_HANDLE m_constantBufferCbvCpuHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_constantBufferCbvGpuHandle;

	// 초기에 Update함수를 먼저 거치기에 read는 0번, write는 1번으로 시작 할수 있도록 초기에 설정
	UINT m_readIdx = 1;
	UINT m_writeIdx = 0;
	UINT m_hashIdx = 2;
	UINT m_scanIdx = 3;
	UINT m_blockIdx = 4;
	UINT m_blockSumIdx = 5;
	UINT m_compactCellIdx = 6;

	void GenerateParticles();
	void CreateStructuredBufferWithViews(
		ComPtr<ID3D12Device>& device, UINT bufferIndex, UINT srvIndex, UINT uavIndex, UINT dataSize, UINT dataCnt, wstring dataName);
	void UploadAndCopyData(ComPtr<ID3D12Device> device,
		ComPtr<ID3D12GraphicsCommandList> commandList, UINT dataSize, ComPtr<ID3D12Resource>& uploadBuffer, wstring dataName, ComPtr<ID3D12Resource>& destBuffer, D3D12_RESOURCE_STATES destBufferState);

	void CalcHashes(ComPtr<ID3D12GraphicsCommandList> commandList);
	void BitonicSort(ComPtr<ID3D12GraphicsCommandList> commandList);
	void CalcHashRange(ComPtr<ID3D12GraphicsCommandList> commandList);
	void FlagGeneration(ComPtr<ID3D12GraphicsCommandList> commandList);
	void FlagScan(ComPtr<ID3D12GraphicsCommandList> commandList);
	void ScatterCompactCell(ComPtr<ID3D12GraphicsCommandList> commandList);
	void CalcSPH(ComPtr<ID3D12GraphicsCommandList> commandList);
};