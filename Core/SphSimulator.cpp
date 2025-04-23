#include "SphSimulator.h"

SphSimulator::SphSimulator()
{
}

SphSimulator::~SphSimulator() {}

void SphSimulator::Initialize(ComPtr<ID3D12Device> device,
	ComPtr<ID3D12GraphicsCommandList> commandList, UINT width, UINT height)
{
	GenerateParticles();

	m_cbvSrvUavSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// ��ũ���� �� ����
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = 2 * STRUCTURED_CNT + CONSTANT_CNT;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_cbvSrvUavHeap)));
	m_cbvSrvUavHeap->SetName(L"SPH SRV/UAV/CBV Heap");

	// StructuredBuffer ����
	CreateStructuredBufferWithViews(device, 0, 0, 1, m_particleDataSize, m_numParticles, L"Particle A");
	CreateStructuredBufferWithViews(device, 1, 2, 3, m_particleDataSize, m_numParticles, L"Particle B");
	CreateStructuredBufferWithViews(device, 2, 4, 5, sizeof(XMFLOAT3), m_numParticles, L"Particle Position");
	CreateStructuredBufferWithViews(device, 3, 6, 7, sizeof(UINT), m_cellCnt, L"CellCount");
	CreateStructuredBufferWithViews(device, 4, 8, 9, sizeof(UINT) * 2, m_numParticles, L"CellOffset");
	CreateStructuredBufferWithViews(device, 5, 10, 11, sizeof(UINT), m_cellCnt, L"CellStart");
	CreateStructuredBufferWithViews(device, 6, 12, 13, sizeof(UINT), (m_cellCnt + m_groupSizeX - 1) / m_groupSizeX, L"CellStartPartialSum");
	CreateStructuredBufferWithViews(device, 7, 14, 15, sizeof(UINT), m_numParticles, L"CellScatter");
	//CreateStructuredBufferWithViews(device, 2, 4, 5, m_particleHashDataSize, m_numParticles, L"ParticleHash");
	//CreateStructuredBufferWithViews(device, 3, 6, 7, m_compactCellDataSize, m_compactCellDataCnt, L"CompactCell");

	// Particle A �ʱ� ���� -> SRV
	UploadAndCopyData(device, commandList, m_particleDataSize, m_particlesUploadBuffer, L"ParticleUploadBuffer", m_structuredBuffer[0], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	// Particle B �ʱ� ���� -> SRV
	SetBarrier(commandList, m_structuredBuffer[1],
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	// Particle Position �ʱ� ���� -> SRV
	UploadAndCopyData(device, commandList, sizeof(XMFLOAT3), m_particlePositionsUploadBuffer, L"ParticlePositionUploadBuffer", m_structuredBuffer[2], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	// CellCount �ʱ� ���� -> UAV
	SetBarrier(commandList, m_structuredBuffer[3],
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	// CellOffset �ʱ� ���� -> UAV
	SetBarrier(commandList, m_structuredBuffer[4],
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	// CellStart �ʱ� ���� -> UAV
	SetBarrier(commandList, m_structuredBuffer[5],
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	// CellStartPartialSum �ʱ� ���� -> UAV
	SetBarrier(commandList, m_structuredBuffer[6],
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	// CellScatter �ʱ� ���� -> UAV
	SetBarrier(commandList, m_structuredBuffer[7],
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	//// Hash �ʱ� ���� -> SRV
	//SetBarrier(commandList, m_structuredBuffer[2],
	//	D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	//// CompactCell �ʱ� ���� -> SRV
	//SetBarrier(commandList, m_structuredBuffer[3],
	//	D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	// ConstantBuffer ����
	m_constantBufferData.deltaTime = 1 / 120.0f;

	m_constantBufferData.minBounds = XMFLOAT3(-m_maxBoundsX, -m_maxBoundsY, -m_maxBoundsZ);
	m_constantBufferData.maxBounds = XMFLOAT3(m_maxBoundsX, m_maxBoundsY, m_maxBoundsZ);
	m_constantBufferData.cellCnt = m_cellCnt;
	m_constantBufferData.smoothingRadius = m_smoothingRadius;
	m_constantBufferData.gravityCoeff = m_gravityCoeff;
	m_constantBufferData.collisionDamping = m_collisionDamping;
	m_constantBufferData.numParticles = m_numParticles;
	m_constantBufferData.gridDimX = m_gridDimX;
	m_constantBufferData.gridDimY = m_gridDimY;
	m_constantBufferData.gridDimZ = m_gridDimZ;

	CreateConstUploadBuffer(device, m_constantBuffer, m_constantBufferData, m_constantBufferDataBegin);

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = m_constantBuffer->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = m_constantBufferSize;

	CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHandle(m_cbvSrvUavHeap->GetCPUDescriptorHandleForHeapStart(), m_cbvSrvUavSize * (2 * STRUCTURED_CNT));
	device->CreateConstantBufferView(
		&cbvDesc,
		cbvHandle
	);

	CD3DX12_GPU_DESCRIPTOR_HANDLE currentGpuHandle(m_cbvSrvUavHeap->GetGPUDescriptorHandleForHeapStart());

	for (UINT i = 0; i < STRUCTURED_CNT; i++)
	{
		m_structuredBufferSrvGpuHandle[i] = currentGpuHandle;
		currentGpuHandle.Offset(1, m_cbvSrvUavSize);
		m_structuredBufferUavGpuHandle[i] = currentGpuHandle;
		currentGpuHandle.Offset(1, m_cbvSrvUavSize);
	}
	m_constantBufferCbvGpuHandle = currentGpuHandle;
}

void SphSimulator::GenerateParticles()
{
	// Particle Data
	m_particles.resize(m_numParticles);
	m_particlePositions.resize(m_numParticles);

	vector<XMFLOAT3> rainbow = {
		{1.0f, 0.0f, 0.0f},  // Red
		{1.0f, 0.0f, 0.0f},  // Red
		{1.0f, 0.65f, 0.0f}, // Orange
		{1.0f, 0.65f, 0.0f}, // Orange
		{1.0f, 1.0f, 0.0f},  // Yellow
		{1.0f, 1.0f, 0.0f},  // Yellow
		{0.0f, 1.0f, 0.0f},  // Green
		{0.0f, 1.0f, 0.0f},  // Green
		{0.0f, 0.0f, 1.0f},  // Blue
		{0.0f, 0.0f, 1.0f},  // Blue
		{0.3f, 0.0f, 0.5f},  // Indigo
		{0.3f, 0.0f, 0.5f},  // Indigo
		{0.5f, 0.0f, 1.0f},  // Violet/Purple
		{0.5f, 0.0f, 1.0f},  // Violet/Purple
		{1.0f, 1.0f, 1.0f},  // White
		{1.0f, 1.0f, 1.0f},  // White
	};

	random_device rd;
	mt19937 gen(rd());
	uniform_real_distribution<float> dpx(-m_maxBoundsX * 2.0f, m_maxBoundsX * 2.0f);
	uniform_real_distribution<float> dpy(-m_maxBoundsY * 2.0f, m_maxBoundsY * 2.0f);
	uniform_real_distribution<float> dl(0.0f, 10.0f);
	uniform_int_distribution<size_t> dc(0, rainbow.size() - 1);

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

				m_particles[index].position.x = minX + m_radius * x;
				m_particles[index].position.y = minY + m_radius * y;
				m_particles[index].position.z = minZ + m_radius * z;

				m_particles[index].life = dl(gen);
				m_particles[index].radius = m_radius;

				m_particlePositions[index].x = minX + m_radius * x;
				m_particlePositions[index].y = minY + m_radius * y;
				m_particlePositions[index].z = minZ + m_radius * z;
			}
		}
	}
}

void SphSimulator::Update(float dt, UINT forceKey)
{
	// �ϴ��� CBV ��ü�� ������Ʈ
	m_constantBufferData.deltaTime = 1 / 100.0f;

	m_constantBufferData.minBounds = XMFLOAT3(-m_maxBoundsX, -m_maxBoundsY, -m_maxBoundsZ);
	m_constantBufferData.maxBounds = XMFLOAT3(m_maxBoundsX, m_maxBoundsY, m_maxBoundsZ);
	m_constantBufferData.cellCnt = m_cellCnt;
	m_constantBufferData.smoothingRadius = m_smoothingRadius;
	m_constantBufferData.gravityCoeff = m_gravityCoeff;
	m_constantBufferData.collisionDamping = m_collisionDamping;
	m_constantBufferData.numParticles = m_numParticles;
	m_constantBufferData.gridDimX = static_cast<UINT>(ceilf(m_maxBoundsX * 2.0f / m_smoothingRadius));
	m_constantBufferData.gridDimY = static_cast<UINT>(ceilf(m_maxBoundsY * 2.0f / m_smoothingRadius));
	m_constantBufferData.gridDimZ = static_cast<UINT>(ceilf(m_maxBoundsZ * 2.0f / m_smoothingRadius));
	m_constantBufferData.forceKey = forceKey;

	memcpy(m_constantBufferDataBegin, &m_constantBufferData, sizeof(m_constantBufferData));

	m_particleAIndex = !m_particleAIndex;
	m_particleBIndex = !m_particleBIndex;
}

void SphSimulator::Compute(ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	commandList->SetComputeRootSignature(Graphics::sphComputeRootSignature.Get());

	ID3D12DescriptorHeap* ppHeap[] = { m_cbvSrvUavHeap.Get() };
	commandList->SetDescriptorHeaps(_countof(ppHeap), ppHeap);

	commandList->SetComputeRootDescriptorTable(8, m_constantBufferCbvGpuHandle); // CBV

	// Clear CellCount
	commandList->SetPipelineState(Graphics::sphClearCountCellCSPSO.Get());

	commandList->SetComputeRootDescriptorTable(4, m_structuredBufferUavGpuHandle[m_cellCountIndex]); // UAV 0

	UINT clearDispatchX = (m_cellCnt + m_groupSizeX - 1) / m_groupSizeX;
	commandList->Dispatch(clearDispatchX, 1, 1);
	
	// CellCount
	commandList->SetPipelineState(Graphics::sphCountCellCSPSO.Get());

	SetUAVBarrier(commandList, m_structuredBuffer[m_cellCountIndex]); // CellCount : UAV -> UAV

	commandList->SetComputeRootDescriptorTable(0, m_structuredBufferSrvGpuHandle[m_particleAIndex]); // SRV 0
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

	//CalcHashes(commandList); // �ؽ� ����
	//BitonicSort(commandList); // �ؽ� �� ���� ����
	//FlagGeneration(commandList);
	//ScatterCompactCell(commandList);
}


void SphSimulator::CalcHashes(ComPtr<ID3D12GraphicsCommandList> commandList)
{
	commandList->SetPipelineState(Graphics::sphCalcHashCSPSO.Get());

	SetBarrier(commandList, m_structuredBuffer[m_hashIdx],	   // HASH : SRV -> UAV
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	commandList->SetComputeRootDescriptorTable(0, m_structuredBufferSrvGpuHandle[m_particleAIndex]); // SRV 0
	commandList->SetComputeRootDescriptorTable(4, m_structuredBufferUavGpuHandle[m_hashIdx]); // UAV 0

	UINT dispatchX = (m_numParticles + m_groupSizeX - 1) / m_groupSizeX;
	commandList->Dispatch(dispatchX, 1, 1);
}

void SphSimulator::BitonicSort(ComPtr<ID3D12GraphicsCommandList> commandList)
{
	commandList->SetPipelineState(Graphics::sphBitonicSortCSPSO.Get());

	SetUAVBarrier(commandList, m_structuredBuffer[m_hashIdx]);

	commandList->SetComputeRootDescriptorTable(4, m_structuredBufferUavGpuHandle[m_hashIdx]); // UAV 0

	UINT dispatchX = (m_numParticles + m_groupSizeX - 1) / m_groupSizeX;

	for (uint32_t k = 2; k <= m_numParticles; k *= 2)
	{
		for (uint32_t j = k / 2; j > 0; j /= 2)
		{
			commandList->SetComputeRoot32BitConstant(9, k, 0);
			commandList->SetComputeRoot32BitConstant(9, j, 1);
			SetUAVBarrier(commandList, m_structuredBuffer[m_hashIdx]);

			commandList->Dispatch(dispatchX, 1, 1);
		}
	}
}

void SphSimulator::FlagGeneration(ComPtr<ID3D12GraphicsCommandList> commandList)
{
	commandList->SetPipelineState(Graphics::sphFlagGenerationCSPSO.Get());

	SetUAVBarrier(commandList, m_structuredBuffer[m_hashIdx]);

	commandList->SetComputeRootDescriptorTable(4, m_structuredBufferUavGpuHandle[m_hashIdx]); // UAV

	UINT dispatchX = (m_numParticles + m_groupSizeX - 1) / m_groupSizeX;
	commandList->Dispatch(dispatchX, 1, 1);
}

void SphSimulator::ScatterCompactCell(ComPtr<ID3D12GraphicsCommandList> commandList)
{

	// ClearCell
	commandList->SetPipelineState(Graphics::sphClearCellCSPSO.Get());

	SetBarrier(commandList, m_structuredBuffer[m_compactCellIdx], // CompactCell : SRV -> UAV
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	commandList->SetComputeRootDescriptorTable(4, m_structuredBufferUavGpuHandle[m_compactCellIdx]);

	UINT cellDispatchX = (m_cellCnt + m_groupSizeX - 1) / m_groupSizeX;
	commandList->Dispatch(cellDispatchX, 1, 1);

	// ScatterCompactCell
	commandList->SetPipelineState(Graphics::sphScatterCompactCellCSPSO.Get());

	SetUAVBarrier(commandList, m_structuredBuffer[m_hashIdx]);    // Hash : UAV -> SRV

	SetUAVBarrier(commandList, m_structuredBuffer[m_compactCellIdx]); // CompactCell : UAV -> UAV

	commandList->SetComputeRootDescriptorTable(0, m_structuredBufferSrvGpuHandle[m_hashIdx]);
	commandList->SetComputeRootDescriptorTable(4, m_structuredBufferUavGpuHandle[m_compactCellIdx]);

	UINT dispatchX = (m_numParticles + m_groupSizeX - 1) / m_groupSizeX;
	commandList->Dispatch(dispatchX, 1, 1);

}

void SphSimulator::CalcDensityForces(ComPtr<ID3D12GraphicsCommandList> commandList)
{
	// Density
	commandList->SetPipelineState(Graphics::sphCalcDensityCSPSO.Get());

	SetBarrier(commandList, m_structuredBuffer[m_particleBIndex], // ParticleB : SRV -> UAV
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	SetUAVBarrier(commandList, m_structuredBuffer[m_cellScatterIndex]); // CellScatter : UAV -> SRV
	SetBarrier(commandList, m_structuredBuffer[m_cellScatterIndex],    
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	commandList->SetComputeRootDescriptorTable(0, m_structuredBufferSrvGpuHandle[m_particleAIndex]); // SRV 0
	commandList->SetComputeRootDescriptorTable(1, m_structuredBufferSrvGpuHandle[m_cellStartIndex]); // SRV 1
	commandList->SetComputeRootDescriptorTable(2, m_structuredBufferSrvGpuHandle[m_cellCountIndex]); // SRV 2
	commandList->SetComputeRootDescriptorTable(3, m_structuredBufferSrvGpuHandle[m_cellScatterIndex]); // SRV 3
	commandList->SetComputeRootDescriptorTable(4, m_structuredBufferUavGpuHandle[m_particleBIndex]); // UAV 0

	UINT dispatchX = (m_numParticles + m_groupSizeX - 1) / m_groupSizeX;
	commandList->Dispatch(dispatchX, 1, 1);

	// Forces
	commandList->SetPipelineState(Graphics::sphCalcForcesCSPSO.Get());

	SetUAVBarrier(commandList, m_structuredBuffer[m_particleBIndex]); // ParticleB : UAV -> SRV
	SetBarrier(commandList, m_structuredBuffer[m_particleBIndex],
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	SetBarrier(commandList, m_structuredBuffer[m_particleAIndex],    // ParticleA : SRV -> UAV
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	commandList->SetComputeRootDescriptorTable(0, m_structuredBufferSrvGpuHandle[m_particleBIndex]); // SRV 0
	commandList->SetComputeRootDescriptorTable(4, m_structuredBufferUavGpuHandle[m_particleAIndex]); // UAV 0

	// ��ũ����  ������ Density�� ����
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
	// SPH ����
	SetUAVBarrier(commandList, m_structuredBuffer[m_particleAIndex]); // ParticleA : UAV -> SRV
	SetBarrier(commandList, m_structuredBuffer[m_particleAIndex],
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	SetBarrier(commandList, m_structuredBuffer[m_particleBIndex],    // ParticleB : SRV -> UAV
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	//SetUAVBarrier(commandList, m_structuredBuffer[m_particlePositionIndex]);

	commandList->SetPipelineState(Graphics::sphCSPSO.Get());

	commandList->SetComputeRootDescriptorTable(0, m_structuredBufferSrvGpuHandle[m_particleAIndex]); // SRV 0
	commandList->SetComputeRootDescriptorTable(4, m_structuredBufferUavGpuHandle[m_particleBIndex]); // UAV 0

	UINT dispatchX = (m_numParticles + m_groupSizeX - 1) / m_groupSizeX;
	commandList->Dispatch(dispatchX, 1, 1);

	SetUAVBarrier(commandList, m_structuredBuffer[m_particleBIndex]);
}

void SphSimulator::Render(ComPtr<ID3D12GraphicsCommandList>& commandList,
	ComPtr<ID3D12Resource>& globalConstsUploadHeap)
{

	SetUAVBarrier(commandList, m_structuredBuffer[m_particleBIndex]); // ParticleB : UAV -> SRV
	SetBarrier(commandList, m_structuredBuffer[m_particleBIndex],
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	ID3D12DescriptorHeap* ppHeap[] = { m_cbvSrvUavHeap.Get() };
	commandList->SetDescriptorHeaps(_countof(ppHeap), ppHeap);
	commandList->SetGraphicsRootDescriptorTable(0, m_structuredBufferSrvGpuHandle[m_particleBIndex]); // SRV 0
	commandList->SetGraphicsRootConstantBufferView(1, globalConstsUploadHeap->GetGPUVirtualAddress()); // CBV

	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
	commandList->DrawInstanced(m_numParticles, 1, 0, 0);
}

// StructuredBuffer�� �����ϰ�, m_cbvSrvUavHeap�� index ��ġ�� SRV, index + 1��ġ�� UAV�� ����
void SphSimulator::CreateStructuredBufferWithViews(
	ComPtr<ID3D12Device>& device, UINT bufferIndex, UINT srvIndex, UINT uavIndex, UINT dataSize, UINT dataCnt, wstring dataName)
{
	UINT particleDataSize = static_cast<UINT>(dataSize * dataCnt);

	auto heapPropsDefault = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	auto bufferDescParticles = CD3DX12_RESOURCE_DESC::Buffer(particleDataSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	// ���� ����
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

	// SRV ���� ����
	CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(m_cbvSrvUavHeap->GetCPUDescriptorHandleForHeapStart(), m_cbvSrvUavSize * srvIndex);

	device->CreateShaderResourceView(
		m_structuredBuffer[bufferIndex].Get(),
		&srvDesc, srvHandle
	);

	// UAV ���� ����
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
		D3D12_RESOURCE_STATE_GENERIC_READ, // UPLOAD ���� GENERIC_READ���� ����
		nullptr,
		IID_PPV_ARGS(&uploadBuffer)));
	uploadBuffer->SetName(dataName.c_str());

	D3D12_SUBRESOURCE_DATA uploadData = {};
	uploadData.pData = m_particles.data();
	uploadData.RowPitch = totalDataSize;
	uploadData.SlicePitch = totalDataSize;

	// ��� ���� ���� ����: COMMON -> COPY_DEST
	SetBarrier(commandList, destBuffer,
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

	UpdateSubresources(commandList.Get(), destBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &uploadData);

	// ��� ���� ���� ����: DEST -> destBufferState
	SetBarrier(commandList, destBuffer,
		D3D12_RESOURCE_STATE_COPY_DEST, destBufferState);

}