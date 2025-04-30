#include "SphSimulator.h"

SphSimulator::SphSimulator()
{
}

SphSimulator::~SphSimulator() {}

void SphSimulator::Initialize(ComPtr<ID3D12Device> device,
	ComPtr<ID3D12GraphicsCommandList> commandList, UINT width, UINT height)
{
	m_particles.resize(m_numParticles);

	//GenerateGhostParticles();
	//GenerateEmitterParticles();
	GenerateDamParticles();

	m_cbvSrvUavSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// 디스크립터 힙 생성
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = 2 * STRUCTURED_CNT + CONSTANT_CNT;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_cbvSrvUavHeap)));
	m_cbvSrvUavHeap->SetName(L"SPH SRV/UAV/CBV Heap");

	// StructuredBuffer 생성
	CreateStructuredBufferWithViews(device, 0, 0, 1, m_particleDataSize, m_numParticles, L"Particle A");
	CreateStructuredBufferWithViews(device, 1, 2, 3, m_particleDataSize, m_numParticles, L"Particle B");
	CreateStructuredBufferWithViews(device, 2, 4, 5, sizeof(UINT), m_cellCnt, L"CellCount");
	CreateStructuredBufferWithViews(device, 3, 6, 7, sizeof(UINT) * 2, m_numParticles, L"CellOffset");
	CreateStructuredBufferWithViews(device, 4, 8, 9, sizeof(UINT), m_cellCnt, L"CellStart");
	CreateStructuredBufferWithViews(device, 5, 10, 11, sizeof(UINT), (m_cellCnt + m_groupSizeX - 1) / m_groupSizeX, L"CellStartPartialSum");
	CreateStructuredBufferWithViews(device, 6, 12, 13, sizeof(UINT), m_numParticles, L"CellScatter");

	// Particle A 초기 상태 -> SRV
	UploadAndCopyData(device, commandList, m_particleDataSize,
		m_particlesUploadBuffer, L"ParticleUploadBuffer", m_structuredBuffer[0],
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	// Particle B 초기 상태 -> UAV
	SetBarrier(commandList, m_structuredBuffer[1],
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	// CellCount 초기 상태 -> UAV
	SetBarrier(commandList, m_structuredBuffer[2],
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	// CellOffset 초기 상태 -> UAV
	SetBarrier(commandList, m_structuredBuffer[3],
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	// CellStart 초기 상태 -> UAV
	SetBarrier(commandList, m_structuredBuffer[4],
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	// CellStartPartialSum 초기 상태 -> UAV
	SetBarrier(commandList, m_structuredBuffer[5],
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	// CellScatter 초기 상태 -> UAV
	SetBarrier(commandList, m_structuredBuffer[6],
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	CreateConstantBuffer(device);

	CD3DX12_GPU_DESCRIPTOR_HANDLE currentGpuHandle(m_cbvSrvUavHeap->GetGPUDescriptorHandleForHeapStart());

	for (UINT i = 0; i < STRUCTURED_CNT; i++)
	{
		m_structuredBufferSrvGpuHandle[i] = currentGpuHandle;
		currentGpuHandle.Offset(1, m_cbvSrvUavSize);
		m_structuredBufferUavGpuHandle[i] = currentGpuHandle;
		currentGpuHandle.Offset(1, m_cbvSrvUavSize);
	}
	m_simParamsCbvGpuHandle = currentGpuHandle;
}

void SphSimulator::GenerateGhostParticles()
{
	UINT ghostCnt = 0;

	UINT bottomCnt = m_wallXCnt * m_wallZCnt;

	// Bottom
	for (UINT z = 0; z < m_wallZCnt; z++)
	{
		for (UINT x = 0; x < m_wallXCnt; x++)
		{
			UINT index = m_numParticles - 1 - (ghostCnt + x + z * m_wallXCnt);
			m_particles[index].isGhost = true;
			m_particles[index].spawnTime = -1.0f;
			m_particles[index].position = XMFLOAT3(-m_maxBoundsX + m_dp * x, -m_maxBoundsY, -m_maxBoundsZ + m_dp * z);
		}
	}
	ghostCnt += bottomCnt;

	// Up
	for (UINT z = 0; z < m_wallZCnt; z++)
	{
		for (UINT x = 0; x < m_wallXCnt; x++)
		{
			UINT index = m_numParticles - 1 - (ghostCnt + x + z * m_wallXCnt);
			m_particles[index].isGhost = true;
			m_particles[index].spawnTime = -1.0f;
			m_particles[index].position = XMFLOAT3(-m_maxBoundsX + m_dp * x, m_maxBoundsY, -m_maxBoundsZ + m_dp * z);
		}
	}
	ghostCnt += bottomCnt;

	UINT sideCnt = m_wallZCnt * m_wallYCnt;

	// Left
	for (UINT y = 0; y < m_wallYCnt; y++)
	{
		for (UINT z = 0; z < m_wallZCnt; z++)
		{
			UINT index = m_numParticles - 1 - (ghostCnt + z + y * m_wallZCnt);
			m_particles[index].isGhost = true;
			m_particles[index].spawnTime = -1.0f;
			m_particles[index].position = XMFLOAT3(-m_maxBoundsX, -m_maxBoundsY + m_dp * y, -m_maxBoundsZ + m_dp * z);

		}
	}
	ghostCnt += sideCnt;

	// Right
	for (UINT y = 0; y < m_wallYCnt; y++)
	{
		for (UINT z = 0; z < m_wallZCnt; z++)
		{
			UINT index = m_numParticles - 1 - (ghostCnt + z + y * m_wallZCnt);
			m_particles[index].isGhost = true;
			m_particles[index].spawnTime = -1.0f;
			m_particles[index].position = XMFLOAT3(m_maxBoundsX, -m_maxBoundsY + m_dp * y, -m_maxBoundsZ + m_dp * z);

		}
	}
	ghostCnt += sideCnt;

	UINT frontCnt = m_wallXCnt * m_wallYCnt;

	// Front
	for (UINT y = 0; y < m_wallYCnt; y++)
	{
		for (UINT x = 0; x < m_wallXCnt; x++)
		{
			UINT index = m_numParticles - 1 - (ghostCnt + x + y * m_wallXCnt);
			m_particles[index].isGhost = true;
			m_particles[index].spawnTime = -1.0f;
			m_particles[index].position = XMFLOAT3(-m_maxBoundsX + m_dp * x, -m_maxBoundsY + m_dp * y, -m_maxBoundsZ);

		}
	}
	ghostCnt += frontCnt;

	// Back
	for (UINT y = 0; y < m_wallYCnt; y++)
	{
		for (UINT x = 0; x < m_wallXCnt; x++)
		{
			UINT index = m_numParticles - 1 - (ghostCnt + x + y * m_wallXCnt);
			m_particles[index].isGhost = true;
			m_particles[index].spawnTime = -1.0f;
			m_particles[index].position = XMFLOAT3(-m_maxBoundsX + m_dp * x, -m_maxBoundsY + m_dp * y, m_maxBoundsZ);
		}
	}
	ghostCnt += frontCnt;
}

void SphSimulator::GenerateEmitterParticles()
{
	//m_particles.resize(m_numParticles);

	float midX = (m_maxBoundsX + -m_maxBoundsX) * 0.5f;
	float midY = (m_maxBoundsY + -m_maxBoundsY) * 0.5f;
	float midZ = (m_maxBoundsZ + -m_maxBoundsZ) * 0.5f;

	XMFLOAT3 centerPos = { midX - m_maxBoundsX * 0.5f, midY + m_maxBoundsY * 0.5f, midZ };
	const float radius1 = m_dp * 4.0f;
	const UINT numParticlesRing1 = 8;
	const float radius2 = m_dp * 8.0f;
	const UINT numParticlesRing2 = 16;
	const UINT batchSize = 1 + numParticlesRing1 + numParticlesRing2;

	for (UINT i = 0; i < m_numParticles; ++i)
	{
		if (m_particles[i].isGhost)
			break;
		UINT groupIdx = i / batchSize;
		UINT subIdx = i % batchSize;

		m_particles[i].spawnTime = (groupIdx + 1) * 0.08f;

		if (subIdx == 0) {
			m_particles[i].position = centerPos;
		}
		else if (0 < subIdx && subIdx < 1 + numParticlesRing1)
		{
			const float angleStep = 2.0f * XM_PI / static_cast<float>(numParticlesRing1);
			float angle = static_cast<float>(i) * angleStep;
			float x = centerPos.x;
			float y = centerPos.y + radius1 * cosf(angle);
			float z = centerPos.z + radius1 * sinf(angle);
			m_particles[i].position = XMFLOAT3{ x, y, z };
		}
		else if (1 + numParticlesRing1 <= subIdx && subIdx < batchSize)
		{
			const float angleStep = 2.0f * XM_PI / static_cast<float>(numParticlesRing2);
			float angle = static_cast<float>(i) * angleStep;
			float x = centerPos.x;
			float y = centerPos.y + radius2 * cosf(angle);
			float z = centerPos.z + radius2 * sinf(angle);
			m_particles[i].position = XMFLOAT3{ x, y, z };
		}

		XMStoreFloat3(&m_particles[i].velocity, XMVector3Normalize(XMVECTOR{ -1.0f, -0.5f, 0.0f }) * 5.0f);
	}
}

void SphSimulator::GenerateDamParticles()
{
	float midX = (m_maxBoundsX + -m_maxBoundsX) * 0.5f;
	float midY = (m_maxBoundsY + -m_maxBoundsY) * 0.5f;
	float midZ = (m_maxBoundsZ + -m_maxBoundsZ) * 0.5f;

	float spacingX = m_nX * m_radius * 0.5f;
	float minX = midX - spacingX;
	float maxX = midX + spacingX;

	float spacingY = m_nY * m_radius * 0.5f;
	float minY = midY - spacingY;
	float maxY = midY + spacingY;

	float spacingZ = m_nZ * m_radius * 0.5f;
	float minZ = midZ - spacingZ;
	float maxZ = midZ + spacingZ;

	for (UINT z = 0; z < m_nZ; z++)
	{
		for (UINT y = 0; y < m_nY; y++)
		{
			for (UINT x = 0; x < m_nX; x++)
			{
				UINT index =
					x +
					m_nX * y +
					m_nX * m_nY * z;

				m_particles[index].position.x = minX + m_dp * x;
				m_particles[index].position.y = minY + m_dp * y;
				m_particles[index].position.z = minZ + m_dp * z;
			}
		}
	}
}

void SphSimulator::Update(float dt, UINT& forceKey)
{
	m_simParamsData.minBounds = XMFLOAT3(-m_maxBoundsX, -m_maxBoundsY, -m_maxBoundsZ);
	m_simParamsData.maxBounds = XMFLOAT3(m_maxBoundsX, m_maxBoundsY, m_maxBoundsZ);
	m_simParamsData.cellCnt = m_cellCnt;
	m_simParamsData.smoothingRadius = m_smoothingRadius;
	m_simParamsData.gravityCoeff = m_gravityCoeff;
	m_simParamsData.numParticles = m_numParticles;
	m_simParamsData.gridDimX = m_gridDimX;
	m_simParamsData.gridDimY = m_gridDimY;
	m_simParamsData.gridDimZ = m_gridDimZ;
	m_simParamsData.currentTime += m_simParamsData.deltaTime;
	m_simParamsData.forceKey = forceKey;

	memcpy(m_simParamsConstantBufferDataBegin, &m_simParamsData, sizeof(m_simParamsData));
}

void SphSimulator::Compute(ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	commandList->SetComputeRootSignature(Graphics::sphComputeRootSignature.Get());

	ID3D12DescriptorHeap* ppHeap[] = { m_cbvSrvUavHeap.Get() };
	commandList->SetDescriptorHeaps(_countof(ppHeap), ppHeap);

	commandList->SetComputeRootDescriptorTable(8, m_simParamsCbvGpuHandle); // SimParams CBV

	// Update KickDrift
	commandList->SetPipelineState(Graphics::sphKickDriftCSPSO.Get());

	SetUAVBarrier(commandList, m_structuredBuffer[m_particleBIndex]); // ParticleB : SRV -> UAV
	SetBarrier(commandList, m_structuredBuffer[m_particleBIndex],
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	commandList->SetComputeRootDescriptorTable(0, m_structuredBufferSrvGpuHandle[m_particleAIndex]); // SRV 0
	commandList->SetComputeRootDescriptorTable(4, m_structuredBufferUavGpuHandle[m_particleBIndex]); // UAV 0

	UINT dispatchX = (m_numParticles + m_groupSizeX - 1) / m_groupSizeX;
	commandList->Dispatch(dispatchX, 1, 1);

	// Clear CellCount
	commandList->SetPipelineState(Graphics::sphClearCountCellCSPSO.Get());

	commandList->SetComputeRootDescriptorTable(4, m_structuredBufferUavGpuHandle[m_cellCountIndex]); // UAV 0

	UINT clearDispatchX = (m_cellCnt + m_groupSizeX - 1) / m_groupSizeX;
	commandList->Dispatch(clearDispatchX, 1, 1);

	// CellCount
	commandList->SetPipelineState(Graphics::sphCountCellCSPSO.Get());

	SetUAVBarrier(commandList, m_structuredBuffer[m_cellCountIndex]); // CellCount : UAV -> UAV
	SetUAVBarrier(commandList, m_structuredBuffer[m_particleBIndex]); // ParticleB : UAV -> SRV
	SetBarrier(commandList, m_structuredBuffer[m_particleBIndex],
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	commandList->SetComputeRootDescriptorTable(0, m_structuredBufferSrvGpuHandle[m_particleBIndex]); // SRV 0
	commandList->SetComputeRootDescriptorTable(4, m_structuredBufferUavGpuHandle[m_cellCountIndex]); // UAV 0
	commandList->SetComputeRootDescriptorTable(5, m_structuredBufferUavGpuHandle[m_cellOffsetIndex]); // UAV 1

	UINT countDispatchX = (m_numParticles + m_groupSizeX - 1) / m_groupSizeX;

	commandList->Dispatch(countDispatchX, 1, 1);

	// Scan
	commandList->SetPipelineState(Graphics::sphCellLocalScanCSPSO.Get());

	SetUAVBarrier(commandList, m_structuredBuffer[m_cellCountIndex]); // CellCount : UAV -> SRV
	SetBarrier(commandList, m_structuredBuffer[m_cellCountIndex],
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	commandList->SetComputeRootDescriptorTable(0, m_structuredBufferSrvGpuHandle[m_cellCountIndex]); // SRV 0 
	commandList->SetComputeRootDescriptorTable(4, m_structuredBufferUavGpuHandle[m_cellStartIndex]); // UAV 0
	commandList->SetComputeRootDescriptorTable(5, m_structuredBufferUavGpuHandle[m_cellStartPartialSumIndex]); // UAV 1

	UINT scanDispatchX = (m_cellCnt + m_groupSizeX - 1) / m_groupSizeX;

	commandList->Dispatch(scanDispatchX, 1, 1);

	// ScanBlock
	commandList->SetPipelineState(Graphics::sphCellLocalScanBlockCSPSO.Get());

	SetUAVBarrier(commandList, m_structuredBuffer[m_cellStartPartialSumIndex]); // CellStartPartialSum : UAV -> UAV

	commandList->SetComputeRootDescriptorTable(4, m_structuredBufferUavGpuHandle[m_cellStartPartialSumIndex]); // UAV 0

	UINT scanBlockDispatchX = ((m_cellCnt + m_groupSizeX - 1) / m_groupSizeX + m_groupSizeX - 1) / m_groupSizeX;

	commandList->Dispatch(scanBlockDispatchX, 1, 1);

	// FinalAddition
	commandList->SetPipelineState(Graphics::sphCellFinalAdditionCSPSO.Get());

	SetUAVBarrier(commandList, m_structuredBuffer[m_cellStartPartialSumIndex]); // CellStartPartialSum : UAV -> SRV
	SetBarrier(commandList, m_structuredBuffer[m_cellStartPartialSumIndex],
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	SetUAVBarrier(commandList, m_structuredBuffer[m_cellStartIndex]); // LocalScan : UAV -> UAV

	commandList->SetComputeRootDescriptorTable(0, m_structuredBufferSrvGpuHandle[m_cellStartPartialSumIndex]); // SRV 0
	commandList->SetComputeRootDescriptorTable(4, m_structuredBufferUavGpuHandle[m_cellStartIndex]); // UAV 0

	UINT finalAddtionDispatchX = (m_cellCnt + m_groupSizeX - 1) / m_groupSizeX;

	commandList->Dispatch(finalAddtionDispatchX, 1, 1);

	// CellScatter
	commandList->SetPipelineState(Graphics::sphCellScatterCSPSO.Get());

	SetUAVBarrier(commandList, m_structuredBuffer[m_cellStartIndex]); // LocalScan : UAV -> SRV
	SetBarrier(commandList, m_structuredBuffer[m_cellStartIndex],
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	SetUAVBarrier(commandList, m_structuredBuffer[m_cellOffsetIndex]); // CellOffset : UAV -> SRV
	SetBarrier(commandList, m_structuredBuffer[m_cellOffsetIndex],
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	commandList->SetComputeRootDescriptorTable(0, m_structuredBufferSrvGpuHandle[m_cellStartIndex]); // SRV 0
	commandList->SetComputeRootDescriptorTable(1, m_structuredBufferSrvGpuHandle[m_cellOffsetIndex]); // SRV 1
	commandList->SetComputeRootDescriptorTable(4, m_structuredBufferUavGpuHandle[m_cellScatterIndex]); // UAV 0

	UINT cellScatterDispatchX = (m_numParticles + m_groupSizeX - 1) / m_groupSizeX;

	commandList->Dispatch(cellScatterDispatchX, 1, 1);

	CalcDensityForces(commandList);
	CalcSPH(commandList);
}

void SphSimulator::CalcDensityForces(ComPtr<ID3D12GraphicsCommandList> commandList)
{
	UINT dispatchX = (m_numParticles + m_groupSizeX - 1) / m_groupSizeX;

	// Density
	commandList->SetPipelineState(Graphics::sphCalcDensityCSPSO.Get());

	SetBarrier(commandList, m_structuredBuffer[m_particleAIndex], // ParticleA : SRV -> UAV
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	SetUAVBarrier(commandList, m_structuredBuffer[m_cellScatterIndex]); // CellScatter : UAV -> SRV
	SetBarrier(commandList, m_structuredBuffer[m_cellScatterIndex],
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	commandList->SetComputeRootDescriptorTable(0, m_structuredBufferSrvGpuHandle[m_particleBIndex]); // SRV 0
	commandList->SetComputeRootDescriptorTable(1, m_structuredBufferSrvGpuHandle[m_cellStartIndex]); // SRV 1
	commandList->SetComputeRootDescriptorTable(2, m_structuredBufferSrvGpuHandle[m_cellCountIndex]); // SRV 2
	commandList->SetComputeRootDescriptorTable(3, m_structuredBufferSrvGpuHandle[m_cellScatterIndex]); // SRV 3
	commandList->SetComputeRootDescriptorTable(4, m_structuredBufferUavGpuHandle[m_particleAIndex]); // UAV 0

	commandList->Dispatch(dispatchX, 1, 1);

	// Forces
	commandList->SetPipelineState(Graphics::sphCalcForcesCSPSO.Get());

	SetUAVBarrier(commandList, m_structuredBuffer[m_particleAIndex]); // ParticleA : UAV -> SRV
	SetBarrier(commandList, m_structuredBuffer[m_particleAIndex],
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	SetBarrier(commandList, m_structuredBuffer[m_particleBIndex],	  // ParticleB : SRV -> UAV
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	commandList->SetComputeRootDescriptorTable(0, m_structuredBufferSrvGpuHandle[m_particleAIndex]); // SRV 0
	commandList->SetComputeRootDescriptorTable(4, m_structuredBufferUavGpuHandle[m_particleBIndex]); // UAV 0

	// 디스크립터  설정은 Density와 동일
	commandList->Dispatch(dispatchX, 1, 1);

	SetBarrier(commandList, m_structuredBuffer[m_cellCountIndex],    // CellCount : SRV -> UAV
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	SetBarrier(commandList, m_structuredBuffer[m_cellOffsetIndex],    // CellOffset : SRV -> UAV
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	SetBarrier(commandList, m_structuredBuffer[m_cellStartIndex],    // CellStart : SRV -> UAV
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	SetBarrier(commandList, m_structuredBuffer[m_cellStartPartialSumIndex],    // CellStartPartialSum : SRV -> UAV
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	SetBarrier(commandList, m_structuredBuffer[m_cellScatterIndex],    // ScatterIndex : SRV -> UAV
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

}

void SphSimulator::CalcSPH(ComPtr<ID3D12GraphicsCommandList> commandList)
{
	// SPH 연산 - Final Kick, 경계조건처리
	SetUAVBarrier(commandList, m_structuredBuffer[m_particleBIndex]); // ParticleB : UAV -> SRV
	SetBarrier(commandList, m_structuredBuffer[m_particleBIndex],
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	SetBarrier(commandList, m_structuredBuffer[m_particleAIndex],    // ParticleA : SRV -> UAV
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	commandList->SetPipelineState(Graphics::sphCSPSO.Get());

	commandList->SetComputeRootDescriptorTable(0, m_structuredBufferSrvGpuHandle[m_particleBIndex]); // SRV 0
	commandList->SetComputeRootDescriptorTable(4, m_structuredBufferUavGpuHandle[m_particleAIndex]); // UAV 0

	UINT dispatchX = (m_numParticles + m_groupSizeX - 1) / m_groupSizeX;
	commandList->Dispatch(dispatchX, 1, 1);

}

void SphSimulator::Render(ComPtr<ID3D12GraphicsCommandList>& commandList,
	ComPtr<ID3D12Resource>& globalConstsUploadHeap)
{
	commandList->SetGraphicsRootSignature(Graphics::sphRenderRootSignature.Get());
	commandList->SetPipelineState(Graphics::sphPSO.Get());

	SetUAVBarrier(commandList, m_structuredBuffer[m_particleAIndex]); // ParticleA : UAV -> SRV
	SetBarrier(commandList, m_structuredBuffer[m_particleAIndex],
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	ID3D12DescriptorHeap* ppHeap[] = { m_cbvSrvUavHeap.Get() };
	commandList->SetDescriptorHeaps(_countof(ppHeap), ppHeap);
	commandList->SetGraphicsRootDescriptorTable(0, m_structuredBufferSrvGpuHandle[m_particleAIndex]); // SRV 0
	commandList->SetGraphicsRootConstantBufferView(1, globalConstsUploadHeap->GetGPUVirtualAddress()); // CBV

	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
	commandList->DrawInstanced(m_numParticles, 1, 0, 0);
}

// StructuredBuffer를 생성하고, m_cbvSrvUavHeap의 index 위치에 SRV, index + 1위치에 UAV를 생성
void SphSimulator::CreateStructuredBufferWithViews(
	ComPtr<ID3D12Device>& device, UINT bufferIndex, UINT srvIndex, UINT uavIndex, UINT dataSize, UINT dataCnt, wstring dataName)
{
	UINT particleDataSize = static_cast<UINT>(dataSize * dataCnt);

	auto heapPropsDefault = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	auto bufferDescParticles = CD3DX12_RESOURCE_DESC::Buffer(particleDataSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	// 버퍼 생성
	ThrowIfFailed(device->CreateCommittedResource(
		&heapPropsDefault,
		D3D12_HEAP_FLAG_NONE,
		&bufferDescParticles,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&m_structuredBuffer[bufferIndex])));
	m_structuredBuffer[bufferIndex]->SetName(dataName.c_str());

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = dataCnt;
	srvDesc.Buffer.StructureByteStride = dataSize;
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	// SRV 버퍼 설정
	CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(m_cbvSrvUavHeap->GetCPUDescriptorHandleForHeapStart(), m_cbvSrvUavSize * srvIndex);

	device->CreateShaderResourceView(
		m_structuredBuffer[bufferIndex].Get(),
		&srvDesc, srvHandle
	);

	// UAV 버퍼 설정
	CD3DX12_CPU_DESCRIPTOR_HANDLE uavHandle(m_cbvSrvUavHeap->GetCPUDescriptorHandleForHeapStart(), m_cbvSrvUavSize * uavIndex);

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.FirstElement = 0;
	uavDesc.Buffer.NumElements = dataCnt;
	uavDesc.Buffer.StructureByteStride = dataSize;
	uavDesc.Buffer.CounterOffsetInBytes = 0;
	uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

	device->CreateUnorderedAccessView(
		m_structuredBuffer[bufferIndex].Get(),
		nullptr, &uavDesc, uavHandle);
}

void SphSimulator::UploadAndCopyData(ComPtr<ID3D12Device> device,
	ComPtr<ID3D12GraphicsCommandList> commandList, UINT dataSize, ComPtr<ID3D12Resource>& uploadBuffer, wstring dataName, ComPtr<ID3D12Resource>& destBuffer, D3D12_RESOURCE_STATES destBufferState)
{
	UINT totalDataSize = dataSize * m_numParticles;
	auto heapPropsUpload = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto bufferDescUpload = CD3DX12_RESOURCE_DESC::Buffer(totalDataSize);
	ThrowIfFailed(device->CreateCommittedResource(
		&heapPropsUpload,
		D3D12_HEAP_FLAG_NONE,
		&bufferDescUpload,
		D3D12_RESOURCE_STATE_GENERIC_READ, // UPLOAD 힙은 GENERIC_READ에서 시작
		nullptr,
		IID_PPV_ARGS(&uploadBuffer)));
	uploadBuffer->SetName(dataName.c_str());

	D3D12_SUBRESOURCE_DATA uploadData = {};
	uploadData.pData = m_particles.data();
	uploadData.RowPitch = totalDataSize;
	uploadData.SlicePitch = totalDataSize;

	// 대상 버퍼 상태 전이: COMMON -> COPY_DEST
	SetBarrier(commandList, destBuffer,
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

	UpdateSubresources(commandList.Get(), destBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &uploadData);

	// 대상 버퍼 상태 전이: DEST -> destBufferState
	SetBarrier(commandList, destBuffer,
		D3D12_RESOURCE_STATE_COPY_DEST, destBufferState);

}

void SphSimulator::CreateConstantBuffer(ComPtr<ID3D12Device> device)
{
	{
		// SimParams 설정
		m_simParamsData.deltaTime = m_deltaTime;
		m_simParamsData.minBounds = XMFLOAT3(-m_maxBoundsX, -m_maxBoundsY, -m_maxBoundsZ);
		m_simParamsData.maxBounds = XMFLOAT3(m_maxBoundsX, m_maxBoundsY, m_maxBoundsZ);
		m_simParamsData.cellCnt = m_cellCnt;
		m_simParamsData.smoothingRadius = m_smoothingRadius;
		m_simParamsData.gravityCoeff = m_gravityCoeff;
		m_simParamsData.numParticles = m_numParticles;
		m_simParamsData.gridDimX = m_gridDimX;
		m_simParamsData.gridDimY = m_gridDimY;
		m_simParamsData.gridDimZ = m_gridDimZ;
		m_simParamsData.collisionDamping = m_collisionDamping;

		CreateConstUploadBuffer(device, m_simParamsConstantBuffer, m_simParamsData, m_simParamsConstantBufferDataBegin);

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = m_simParamsConstantBuffer->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = m_simParamsConstantBufferSize;

		CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHandle(m_cbvSrvUavHeap->GetCPUDescriptorHandleForHeapStart(), m_cbvSrvUavSize * (2 * STRUCTURED_CNT));
		device->CreateConstantBufferView(
			&cbvDesc,
			cbvHandle
		);
	}
}