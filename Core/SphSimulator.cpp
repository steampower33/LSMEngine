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
	m_particleDataSize = sizeof(Particle);
	m_particleHashDataSize = sizeof(ParticleHash);
	m_localScanDataSize = sizeof(ScanResult);
	m_testGroupSizeX = 4;
	m_testCnt = static_cast<UINT>(m_maxParticles / m_testGroupSizeX);

	// 디스크립터 힙 생성
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = 2 * STRUCTURED_CNT + CONSTANT_CNT;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_cbvSrvUavHeap)));
	m_cbvSrvUavHeap->SetName(L"SPH SRV/UAV/CBV Heap");

	// StructuredBuffer 생성
	CreateStructuredBufferWithViews(device, 0, 0, 1, m_particleDataSize, m_maxParticles, L"Particle 0");
	CreateStructuredBufferWithViews(device, 1, 2, 3, m_particleDataSize, m_maxParticles, L"Particle 1");
	CreateStructuredBufferWithViews(device, 2, 4, 5, m_particleHashDataSize, m_maxParticles, L"ParticleHash");
	CreateStructuredBufferWithViews(device, 3, 6, 7, m_localScanDataSize, m_maxParticles, L"LocalScan");
	CreateStructuredBufferWithViews(device, 4, 8, 9, m_localScanDataSize, m_testCnt, L"PartialSum 0");
	CreateStructuredBufferWithViews(device, 5, 10, 11, m_localScanDataSize, m_testCnt, L"PartialSum 1");

	// 파티클 데이터 업로드
	// buffer 0 초기 상태 -> UAV
	UploadAndCopyData(device, commandList, m_particleDataSize, m_particlesUploadBuffer, L"ParticleUploadBuffer", m_structuredBuffer[0], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	// Buffer 1 초기 상태 - COMMON -> SRV
	SetBarrier(commandList, m_structuredBuffer[1],
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	// 파티클 해시 데이터 업로드
	// buffer 2(Hash) 초기 상태 -> SRV
	SetBarrier(commandList, m_structuredBuffer[2],
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	// LocalScan 초기 상태 -> SRV
	SetBarrier(commandList, m_structuredBuffer[3],
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	// PartialSum 초기 상태 -> SRV
	SetBarrier(commandList, m_structuredBuffer[4],
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	SetBarrier(commandList, m_structuredBuffer[5],
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	// ConstantBuffer 설정
	m_constantBufferData.minBounds = XMFLOAT3(minBounds[0], minBounds[1], minBounds[2]);
	m_constantBufferData.maxBounds = XMFLOAT3(maxBounds[0], maxBounds[1], maxBounds[2]);
	m_constantBufferData.cellSize = cellSize;
	m_constantBufferData.gridDimX = static_cast<UINT>((maxBounds[0] - minBounds[0]) / cellSize);
	m_constantBufferData.gridDimY = static_cast<UINT>((maxBounds[1] - minBounds[1]) / cellSize);
	m_constantBufferData.gridDimZ = static_cast<UINT>((maxBounds[2] - minBounds[2]) / cellSize);

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
		{1.0f, 0.65f, 0.0f}, // Orange
		{1.0f, 1.0f, 0.0f},  // Yellow
		{0.0f, 1.0f, 0.0f},  // Green
		{0.0f, 0.0f, 1.0f},  // Blue
		{0.3f, 0.0f, 0.5f},  // Indigo
		{0.5f, 0.0f, 1.0f}   // Violet/Purple
	};

	random_device rd;
	mt19937 gen(rd());
	uniform_real_distribution<float> dp(-1.0f, 1.0f);
	uniform_int_distribution<size_t> dc(0, rainbow.size() - 1);

	for (UINT i = 0; i < m_maxParticles; i++)
	{
		m_particles[i].position.x = minBounds[0] +
			(maxBounds[0] - minBounds[0] - (maxBounds[0] - minBounds[0]) / 16.0f) / 16.0f * (1 + (i % 16));
		m_particles[i].position.y = minBounds[1] + 
			(maxBounds[1] - minBounds[1] - (maxBounds[1] - minBounds[1]) / 16.0f) / 16.0f * (1 + (i / 16));
		m_particles[i].size = radius;
	}

	//// Particle Hash Data
	//m_particlesHashes.resize(m_maxParticles);

	//// Particel Sorted Hash Group ID
	//m_localScan.resize(m_maxParticles);

	//m_partialSums.resize(m_maxParticles / m_partialTestSize);
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

	// 대상 버퍼 상태 전이: COMMON -> destBufferState
	SetBarrier(commandList, destBuffer,
		D3D12_RESOURCE_STATE_COPY_DEST, destBufferState);

}

void SphSimulator::Update(float dt)
{
	// 일단은 CBV 전체를 업데이트
	m_constantBufferData.deltaTime = dt;
	m_constantBufferData.minBounds = XMFLOAT3(minBounds[0], minBounds[1], minBounds[2]);
	m_constantBufferData.maxBounds = XMFLOAT3(maxBounds[0], maxBounds[1], maxBounds[2]);
	m_constantBufferData.cellSize = cellSize;
	m_constantBufferData.gridDimX = static_cast<UINT>((maxBounds[0] - minBounds[0]) / cellSize);
	m_constantBufferData.gridDimY = static_cast<UINT>((maxBounds[1] - minBounds[1]) / cellSize);
	m_constantBufferData.gridDimZ = static_cast<UINT>((maxBounds[2] - minBounds[2]) / cellSize);
	
	memcpy(m_constantBufferDataBegin, &m_constantBufferData, sizeof(m_constantBufferData));

	// 읽기 쓰기 인덱스 갱신
	m_readIdx = !m_readIdx;
	m_writeIdx = !m_writeIdx;
}

void SphSimulator::Compute(ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	commandList ->SetComputeRootSignature(Graphics::sphComputeRootSignature.Get());

	ID3D12DescriptorHeap* ppHeap[] = { m_cbvSrvUavHeap.Get() };
	commandList->SetDescriptorHeaps(_countof(ppHeap), ppHeap);

	CalcHashes(commandList); // 해시 연산
	BitonicSort(commandList); // 해시 값 기준 정렬
	CalcHashRange(commandList); // 해시 배열로부터 셀 범위를 기록
	CalcSPH(commandList);
}

void SphSimulator::CalcHashes(ComPtr<ID3D12GraphicsCommandList> commandList)
{
	commandList->SetPipelineState(Graphics::sphCalcHashCSPSO.Get());

	// m_particleBuffers[readIdx] -> UAV -> SRV
	// SRV로 사용하기전 UAV 쓰기 작업 기다림
	SetUAVBarrier(commandList, m_structuredBuffer[m_readIdx]);
	SetBarrier(commandList, m_structuredBuffer[m_readIdx],
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	SetBarrier(commandList, m_structuredBuffer[m_hashIdx],
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	commandList->SetComputeRootDescriptorTable(0, m_particleBufferSrvGpuHandle[m_readIdx]); // SRV
	commandList->SetComputeRootDescriptorTable(1, m_particleBufferUavGpuHandle[m_hashIdx]); // UAV
	commandList->SetComputeRootDescriptorTable(4, m_constantBufferCbvGpuHandle); // CBV

	const UINT groupSizeX = m_maxParticles;
	UINT dispatchX = (m_maxParticles + groupSizeX - 1) / groupSizeX;
	commandList->Dispatch(dispatchX, 1, 1);
}

// 만약 상수 버퍼를 사용하고싶다면,
// Multiframe-Inflite 방식일때는 프레임 개수만큼 만들어서 사용해야함.
// 지금은 루트 상수를 사용해서 처리
void SphSimulator::BitonicSort(ComPtr<ID3D12GraphicsCommandList> commandList)
{
	commandList->SetPipelineState(Graphics::sphBitonicSortCSPSO.Get());

	commandList->SetComputeRootDescriptorTable(1, m_particleBufferUavGpuHandle[m_hashIdx]); // UAV

	const UINT groupSizeX = m_maxParticles;
	UINT dispatchX = (m_maxParticles + groupSizeX - 1) / groupSizeX;

	for (uint32_t k = 2; k <= m_maxParticles; k *= 2)
	{
		for (uint32_t j = k / 2; j > 0; j /= 2)
		{
			commandList->SetComputeRoot32BitConstant(5, k, 0);
			commandList->SetComputeRoot32BitConstant(5, j, 1);
			SetUAVBarrier(commandList, m_structuredBuffer[m_hashIdx]);

			commandList->Dispatch(dispatchX, 1, 1);
		}
	}
}

void SphSimulator::CalcHashRange(ComPtr<ID3D12GraphicsCommandList> commandList)
{
	FlagGeneration(commandList);
	FlagScan(commandList);
	//ScatterCellInfo(commandList);
	//CalcEndIndices(commandList);
}

void SphSimulator::FlagGeneration(ComPtr<ID3D12GraphicsCommandList> commandList)
{
	commandList->SetPipelineState(Graphics::sphFlagGenerationCSPSO.Get());

	commandList->SetComputeRootDescriptorTable(1, m_particleBufferUavGpuHandle[m_hashIdx]); // UAV
	SetUAVBarrier(commandList, m_structuredBuffer[m_hashIdx]);

	const UINT groupSizeX = m_maxParticles;
	UINT dispatchX = (m_maxParticles + groupSizeX - 1) / groupSizeX;
	commandList->Dispatch(dispatchX, 1, 1);
}

void SphSimulator::FlagScan(ComPtr<ID3D12GraphicsCommandList> commandList)
{
	// LocalScan
	commandList->SetPipelineState(Graphics::sphLocalScanCSPSO.Get());

	// SRV - SortedHash
	SetUAVBarrier(commandList, m_structuredBuffer[m_hashIdx]);
	// UAV - LocalScanResults
	SetBarrier(commandList, m_structuredBuffer[m_scanIdx],
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	SetBarrier(commandList, m_structuredBuffer[m_partialIdx],
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	commandList->SetComputeRootDescriptorTable(0, m_particleBufferSrvGpuHandle[m_hashIdx]);
	commandList->SetComputeRootDescriptorTable(1, m_particleBufferUavGpuHandle[m_scanIdx]);
	commandList->SetComputeRootDescriptorTable(3, m_particleBufferUavGpuHandle[m_partialIdx]);

	const UINT groupSizeX = 4;
	UINT dispatchX = (m_maxParticles + groupSizeX - 1) / groupSizeX;
	commandList->Dispatch(dispatchX, 1, 1);
}

void SphSimulator::CalcSPH(ComPtr<ID3D12GraphicsCommandList> commandList)
{
	// SPH 연산
	commandList->SetPipelineState(Graphics::sphCSPSO.Get());

	// 쓰기 버퍼 상태 -> SRV -> UAV
	SetBarrier(commandList, m_structuredBuffer[m_writeIdx],
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	// Hash 버퍼 UAV -> SRV
	SetUAVBarrier(commandList, m_structuredBuffer[m_hashIdx]);
	SetBarrier(commandList, m_structuredBuffer[m_hashIdx],
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	
	commandList->SetComputeRootDescriptorTable(0, m_particleBufferSrvGpuHandle[m_readIdx]); // SRV
	commandList->SetComputeRootDescriptorTable(1, m_particleBufferUavGpuHandle[m_writeIdx]); // UAV
	commandList->SetComputeRootDescriptorTable(2, m_constantBufferCbvGpuHandle); // CBV
	commandList->SetComputeRootDescriptorTable(3, m_particleBufferSrvGpuHandle[2]); // SRV - Hash

	const UINT groupSizeX = m_maxParticles;
	UINT dispatchX = (m_maxParticles + groupSizeX - 1) / groupSizeX;
	commandList->Dispatch(dispatchX, 1, 1);
}

void SphSimulator::Render(ComPtr<ID3D12GraphicsCommandList>& commandList,
	ComPtr<ID3D12Resource>& globalConstsUploadHeap)
{
	// Sph Compute 쓰기 버퍼 -> SRV
	SetUAVBarrier(commandList, m_structuredBuffer[m_writeIdx]);
	SetBarrier(commandList, m_structuredBuffer[m_writeIdx],
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	ID3D12DescriptorHeap* ppHeap[] = { m_cbvSrvUavHeap.Get() };
	commandList->SetDescriptorHeaps(_countof(ppHeap), ppHeap);
	commandList->SetGraphicsRootDescriptorTable(0, m_particleBufferSrvGpuHandle[m_writeIdx]); // Structured
	commandList->SetGraphicsRootConstantBufferView(1, globalConstsUploadHeap->GetGPUVirtualAddress()); // Global

	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
	commandList->DrawInstanced(m_maxParticles, 1, 0, 0);

	//// 상태 다시 되돌리기
	//SetBarrier(commandList, m_particleBuffers[m_writeIdx],
	//	D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
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

