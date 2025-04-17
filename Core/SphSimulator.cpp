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

	// 디스크립터 힙 생성
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = 2 * STRUCTURED_CNT + CONSTANT_CNT;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_cbvSrvUavHeap)));
	m_cbvSrvUavHeap->SetName(L"SPH SRV/UAV/CBV Heap");

	// StructuredBuffer 생성
	CreateStructuredBufferWithViews(device, 0, 0, 1, m_particleDataSize, m_maxParticles, L"Particle A");
	CreateStructuredBufferWithViews(device, 1, 2, 3, m_particleDataSize, m_maxParticles, L"Particle B");
	CreateStructuredBufferWithViews(device, 2, 4, 5, m_particleHashDataSize, m_maxParticles, L"ParticleHash");
	CreateStructuredBufferWithViews(device, 3, 6, 7, m_compactCellDataSize, m_compactCellDataCnt, L"CompactCell");

	// Particle A 초기 상태 -> SRV
	UploadAndCopyData(device, commandList, m_particleDataSize, m_particlesUploadBuffer, L"ParticleUploadBuffer", m_structuredBuffer[0], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	// Particle B 초기 상태 -> SRV
	SetBarrier(commandList, m_structuredBuffer[1],
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	// Hash 초기 상태 -> SRV
	SetBarrier(commandList, m_structuredBuffer[2],
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	// CompactCell 초기 상태 -> SRV
	SetBarrier(commandList, m_structuredBuffer[3],
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	// ConstantBuffer 설정
	m_constantBufferData.numParticles = m_maxParticles;
	m_constantBufferData.minBounds = XMFLOAT3(-m_maxBoundsX, -m_maxBoundsY, 0.0f);
	m_constantBufferData.maxBounds = XMFLOAT3(m_maxBoundsX, m_maxBoundsY, 0.0f);
	m_constantBufferData.smoothingRadius = m_smoothingRadius;
	m_constantBufferData.gridDimX = static_cast<UINT>(m_maxBoundsX * 2.0f / m_smoothingRadius);
	m_constantBufferData.gridDimY = static_cast<UINT>(m_maxBoundsY * 2.0f / m_smoothingRadius);
	m_constantBufferData.gridDimZ = static_cast<UINT>(m_maxBoundsZ * 2.0f / m_smoothingRadius);

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
		m_particleBufferSrvGpuHandle[i] = currentGpuHandle;
		currentGpuHandle.Offset(1, m_cbvSrvUavSize);
		m_particleBufferUavGpuHandle[i] = currentGpuHandle;
		currentGpuHandle.Offset(1, m_cbvSrvUavSize);
	}
	m_constantBufferCbvGpuHandle = currentGpuHandle;

}

void SphSimulator::GenerateParticles()
{
	// Particle Data
	m_particles.resize(m_maxParticles);

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

	float spacingX = m_nX * m_radius;
	float minX = midX - spacingX;
	float maxX = midX + spacingX;

	float spacingY = m_nY * m_radius;
	float minY = midY - spacingY;
	float maxY = midY + spacingY;

	for (UINT i = 0; i < m_maxParticles; i++)
	{
		// 중간 위치에서
		m_particles[i].position.x = minX +
			(maxX - minX - (maxX - minX) / m_nX) / m_nX * (1 + (i % m_nX));
		m_particles[i].position.y = minY +
			(maxY - minY - (maxY - minY) / m_nY) / m_nY * (1 + (i / m_nX));

		// 좌측
		//m_particles[i].position.x = -m_maxBoundsX * 0.9f + m_dp * (i % m_nX);
		//m_particles[i].position.y = -m_maxBoundsY * 0.9f + m_dp * (i / m_nX);
		//m_particles[i].predictedPosition.x = m_particles[i].position.x;
		//m_particles[i].predictedPosition.y = m_particles[i].position.y;

		/*m_particles[i].position.x = dpx(gen);
		m_particles[i].position.y = dpy(gen);*/
		//m_particles[i].color = rainbow[i % 16];
		m_particles[i].life = dl(gen);
		m_particles[i].radius = m_radius;
	}
}

void SphSimulator::Update(float dt, UINT forceKey)
{
	// 일단은 CBV 전체를 업데이트
	m_constantBufferData.deltaTime = 1 / 100.0f;

	m_constantBufferData.minBounds = XMFLOAT3(-m_maxBoundsX, -m_maxBoundsY, 0.0f);
	m_constantBufferData.maxBounds = XMFLOAT3(m_maxBoundsX, m_maxBoundsY, 0.0f);
	m_constantBufferData.cellCnt = m_cellCnt;
	m_constantBufferData.smoothingRadius = m_smoothingRadius;
	m_constantBufferData.gravity = m_gravityCoeff;
	m_constantBufferData.collisionDamping = m_collisionDamping;
	m_constantBufferData.maxParticles = m_maxParticles;
	m_constantBufferData.gridDimX = static_cast<UINT>(m_maxBoundsX * 2.0f / m_smoothingRadius);
	m_constantBufferData.gridDimY = static_cast<UINT>(m_maxBoundsY * 2.0f / m_smoothingRadius);
	m_constantBufferData.gridDimZ = static_cast<UINT>(m_maxBoundsZ * 2.0f / m_smoothingRadius);
	m_constantBufferData.forceKey = forceKey;

	memcpy(m_constantBufferDataBegin, &m_constantBufferData, sizeof(m_constantBufferData));

	m_particleA = !m_particleA;
	m_particleB = !m_particleB;
}

void SphSimulator::Compute(ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	commandList->SetComputeRootSignature(Graphics::sphComputeRootSignature.Get());

	ID3D12DescriptorHeap* ppHeap[] = { m_cbvSrvUavHeap.Get() };
	commandList->SetDescriptorHeaps(_countof(ppHeap), ppHeap);

	commandList->SetComputeRootDescriptorTable(8, m_constantBufferCbvGpuHandle); // CBV

	CalcHashes(commandList); // 해시 연산
	BitonicSort(commandList); // 해시 값 기준 정렬
	FlagGeneration(commandList);
	ScatterCompactCell(commandList);
	CalcDensityForces(commandList);
	CalcSPH(commandList);
}


void SphSimulator::CalcHashes(ComPtr<ID3D12GraphicsCommandList> commandList)
{
	commandList->SetPipelineState(Graphics::sphCalcHashCSPSO.Get());

	SetBarrier(commandList, m_structuredBuffer[m_hashIdx],	   // HASH : SRV -> UAV
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	commandList->SetComputeRootDescriptorTable(0, m_particleBufferSrvGpuHandle[m_particleA]); // SRV 0
	commandList->SetComputeRootDescriptorTable(4, m_particleBufferUavGpuHandle[m_hashIdx]); // UAV 0

	UINT dispatchX = (m_maxParticles + m_groupSizeX - 1) / m_groupSizeX;
	commandList->Dispatch(dispatchX, 1, 1);
}

void SphSimulator::BitonicSort(ComPtr<ID3D12GraphicsCommandList> commandList)
{
	commandList->SetPipelineState(Graphics::sphBitonicSortCSPSO.Get());

	SetUAVBarrier(commandList, m_structuredBuffer[m_hashIdx]);

	commandList->SetComputeRootDescriptorTable(4, m_particleBufferUavGpuHandle[m_hashIdx]); // UAV 0

	UINT dispatchX = (m_maxParticles + m_groupSizeX - 1) / m_groupSizeX;

	for (uint32_t k = 2; k <= m_maxParticles; k *= 2)
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

	commandList->SetComputeRootDescriptorTable(4, m_particleBufferUavGpuHandle[m_hashIdx]); // UAV

	UINT dispatchX = (m_maxParticles + m_groupSizeX - 1) / m_groupSizeX;
	commandList->Dispatch(dispatchX, 1, 1);
}

void SphSimulator::ScatterCompactCell(ComPtr<ID3D12GraphicsCommandList> commandList)
{

	// CellMap
	commandList->SetPipelineState(Graphics::sphClearCellCSPSO.Get());

	SetBarrier(commandList, m_structuredBuffer[m_compactCellIdx], // CompactCell : SRV -> UAV
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	commandList->SetComputeRootDescriptorTable(4, m_particleBufferUavGpuHandle[m_compactCellIdx]);

	UINT cellDispatchX = (m_cellCnt + m_groupSizeX - 1) / m_groupSizeX;
	commandList->Dispatch(cellDispatchX, 1, 1);

	// ScatterCompactCell
	commandList->SetPipelineState(Graphics::sphScatterCompactCellCSPSO.Get());

	SetUAVBarrier(commandList, m_structuredBuffer[m_hashIdx]);    // Hash : UAV -> SRV

	SetUAVBarrier(commandList, m_structuredBuffer[m_compactCellIdx]); // CompactCell : UAV -> UAV

	commandList->SetComputeRootDescriptorTable(0, m_particleBufferSrvGpuHandle[m_hashIdx]);
	commandList->SetComputeRootDescriptorTable(4, m_particleBufferUavGpuHandle[m_compactCellIdx]);

	UINT dispatchX = (m_maxParticles + m_groupSizeX - 1) / m_groupSizeX;
	commandList->Dispatch(dispatchX, 1, 1);

}

void SphSimulator::CalcDensityForces(ComPtr<ID3D12GraphicsCommandList> commandList)
{
	// Density
	commandList->SetPipelineState(Graphics::sphCalcDensityCSPSO.Get());

	SetBarrier(commandList, m_structuredBuffer[m_particleB], // ParticleB : SRV -> UAV
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	SetUAVBarrier(commandList, m_structuredBuffer[m_compactCellIdx]); // CompactCell : UAV -> SRV
	SetBarrier(commandList, m_structuredBuffer[m_compactCellIdx],
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	commandList->SetComputeRootDescriptorTable(0, m_particleBufferSrvGpuHandle[m_particleA]); // SRV 0
	commandList->SetComputeRootDescriptorTable(1, m_particleBufferSrvGpuHandle[m_hashIdx]); // SRV 1
	commandList->SetComputeRootDescriptorTable(2, m_particleBufferSrvGpuHandle[m_compactCellIdx]); // SRV 2
	commandList->SetComputeRootDescriptorTable(4, m_particleBufferUavGpuHandle[m_particleB]); // UAV 0

	UINT dispatchX = (m_maxParticles + m_groupSizeX - 1) / m_groupSizeX;
	commandList->Dispatch(dispatchX, 1, 1);

	SetUAVBarrier(commandList, m_structuredBuffer[m_particleB]); // ParticleB : UAV -> SRV

	// Forces
	commandList->SetPipelineState(Graphics::sphCalcForcesCSPSO.Get());

	SetBarrier(commandList, m_structuredBuffer[m_particleB],
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	SetBarrier(commandList, m_structuredBuffer[m_particleA],    // ParticleA : SRV -> UAV
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	commandList->SetComputeRootDescriptorTable(0, m_particleBufferSrvGpuHandle[m_particleB]); // SRV 0
	commandList->SetComputeRootDescriptorTable(4, m_particleBufferUavGpuHandle[m_particleA]); // UAV 0

	// 디스크립터  설정은 Density와 동일
	commandList->Dispatch(dispatchX, 1, 1);
}

void SphSimulator::CalcSPH(ComPtr<ID3D12GraphicsCommandList> commandList)
{
	// SPH 연산
	SetUAVBarrier(commandList, m_structuredBuffer[m_particleA]); // ParticleA : UAV -> SRV
	SetBarrier(commandList, m_structuredBuffer[m_particleA],
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	SetBarrier(commandList, m_structuredBuffer[m_particleB],    // ParticleB : SRV -> UAV
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	commandList->SetPipelineState(Graphics::sphCSPSO.Get());

	commandList->SetComputeRootDescriptorTable(0, m_particleBufferSrvGpuHandle[m_particleA]); // SRV 0
	commandList->SetComputeRootDescriptorTable(4, m_particleBufferUavGpuHandle[m_particleB]); // UAV 0

	UINT dispatchX = (m_maxParticles + m_groupSizeX - 1) / m_groupSizeX;
	commandList->Dispatch(dispatchX, 1, 1);

}

void SphSimulator::Render(ComPtr<ID3D12GraphicsCommandList>& commandList,
	ComPtr<ID3D12Resource>& globalConstsUploadHeap)
{

	SetUAVBarrier(commandList, m_structuredBuffer[m_particleB]); // ParticleB : UAV -> SRV
	SetBarrier(commandList, m_structuredBuffer[m_particleB],
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	ID3D12DescriptorHeap* ppHeap[] = { m_cbvSrvUavHeap.Get() };
	commandList->SetDescriptorHeaps(_countof(ppHeap), ppHeap);
	commandList->SetGraphicsRootDescriptorTable(0, m_particleBufferSrvGpuHandle[m_particleB]); // SRV 0
	commandList->SetGraphicsRootConstantBufferView(1, globalConstsUploadHeap->GetGPUVirtualAddress()); // CBV

	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
	commandList->DrawInstanced(m_maxParticles, 1, 0, 0);
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
	UINT totalDataSize = dataSize * m_maxParticles;
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