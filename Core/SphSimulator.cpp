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

	// 디스크립터 힙 생성
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = 2 + 2 + 1 + 2;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_cbvSrvUavHeap)));
	m_cbvSrvUavHeap->SetName(L"SPH SRV/UAV/CBV Heap");

	// StructuredBuffer 생성
	CreateStructuredBufferWithViews(device, 0, 0, 1, m_particleDataSize, L"Particle");
	CreateStructuredBufferWithViews(device, 1, 2, 3, m_particleDataSize, L"Particle");
	CreateStructuredBufferWithViews(device, 2, 5, 6, m_particleHashDataSize, L"ParticleHash");

	// 파티클 데이터 업로드
	// buffer 0 초기 상태 -> UAV
	UploadAndCopyData(device, commandList, m_particleDataSize, m_particlesUploadBuffer, L"Particle", m_particleBuffers[0], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	// 파티클 해시 데이터 업로드
	// buffer 2(Hash) 초기 상태 -> SRV
	UploadAndCopyData(device, commandList, m_particleHashDataSize, m_particlesHashUploadBuffer, L"ParticleHash", m_particleBuffers[2], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	// Buffer 1 초기 상태 - COMMON -> SRV
	SetBarrier(commandList, m_particleBuffers[1],
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

	CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHandle(m_cbvSrvUavHeap->GetCPUDescriptorHandleForHeapStart(), m_cbvSrvUavSize * 4);
	device->CreateConstantBufferView(
		&cbvDesc,
		cbvHandle
	);

	CD3DX12_GPU_DESCRIPTOR_HANDLE currentGpuHandle(m_cbvSrvUavHeap->GetGPUDescriptorHandleForHeapStart());

	m_particleBufferSrvGpuHandle[0] = currentGpuHandle;
	currentGpuHandle.Offset(1, m_cbvSrvUavSize);
	m_particleBufferUavGpuHandle[0] = currentGpuHandle;
	currentGpuHandle.Offset(1, m_cbvSrvUavSize);
	m_particleBufferSrvGpuHandle[1] = currentGpuHandle;
	currentGpuHandle.Offset(1, m_cbvSrvUavSize);
	m_particleBufferUavGpuHandle[1] = currentGpuHandle;
	currentGpuHandle.Offset(1, m_cbvSrvUavSize);
	m_constantBufferCbvGpuHandle = currentGpuHandle;
	currentGpuHandle.Offset(1, m_cbvSrvUavSize);
	m_particleBufferSrvGpuHandle[2] = currentGpuHandle;
	currentGpuHandle.Offset(1, m_cbvSrvUavSize);
	m_particleBufferUavGpuHandle[2] = currentGpuHandle;

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

	for (auto& p : m_particles) {
		p.position = XMFLOAT3(0.0f, 0.0f, 0.0f);
		p.velocity = XMFLOAT3(0.0f, 0.0f, 0.0);
		p.color = XMFLOAT3(1.0f, 1.0f, 1.0f);
		p.size = radius;
		p.life = -1.0f;
	}

	// Particle Hash Data
	m_particlesHashes.resize(m_maxParticles);

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
	wstring bufferName = dataName + L"UploadBuffer";
	uploadBuffer->SetName(bufferName.c_str());

	D3D12_SUBRESOURCE_DATA uploadData = {};
	uploadData.pData = m_particles.data();
	uploadData.RowPitch = totalDataSize;
	uploadData.SlicePitch = totalDataSize;

	// 대상 버퍼 상태 전이: COMMON -> COPY_DEST
	SetBarrier(commandList, destBuffer,
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

	UpdateSubresources(commandList.Get(), m_particleBuffers[0].Get(), uploadBuffer.Get(), 0, 0, 1, &uploadData);

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
	BitonicSort(commandList);
	CalcSPH(commandList);
}

void SphSimulator::CalcHashes(ComPtr<ID3D12GraphicsCommandList> commandList)
{
	commandList->SetPipelineState(Graphics::sphCalcHashCSPSO.Get());

	// m_particleBuffers[readIdx] -> UAV -> SRV
	// SRV로 사용하기전 UAV 쓰기 작업 기다림
	SetUAVBarrier(commandList, m_particleBuffers[m_readIdx]);
	SetBarrier(commandList, m_particleBuffers[m_readIdx],
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	SetBarrier(commandList, m_particleBuffers[m_hashIdx],
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	commandList->SetComputeRootDescriptorTable(0, m_particleBufferSrvGpuHandle[m_readIdx]); // SRV
	commandList->SetComputeRootDescriptorTable(1, m_particleBufferUavGpuHandle[m_hashIdx]); // UAV
	commandList->SetComputeRootDescriptorTable(2, m_constantBufferCbvGpuHandle); // CBV

	const UINT groupSizeX = 256;
	UINT dispatchX = (m_maxParticles + groupSizeX - 1) / groupSizeX;
	commandList->Dispatch(dispatchX, 1, 1);
}

void SphSimulator::BitonicSort(ComPtr<ID3D12GraphicsCommandList> commandList)
{
	// 만약 상수 버퍼를 사용하고싶다면,
	// Multiframe-Inflite 방식일때는 프레임 개수만큼 만들어서 사용해야함.
	// 지금은 루트 상수를 사용해서 처리

	commandList->SetPipelineState(Graphics::bitonicSortCSPSO.Get());

	commandList->SetComputeRootDescriptorTable(1, m_particleBufferUavGpuHandle[m_hashIdx]); // UAV

	UINT groupSizeX = 256; // HLSL의 numthreads와 일치
	UINT dispatchX = (m_maxParticles + groupSizeX - 1) / groupSizeX;

	for (uint32_t k = 2; k <= m_maxParticles; k *= 2)
	{
		for (uint32_t j = k / 2; j > 0; j /= 2)
		{
			commandList->SetComputeRoot32BitConstant(4, k, 0);
			commandList->SetComputeRoot32BitConstant(4, j, 1);
			SetUAVBarrier(commandList, m_particleBuffers[m_hashIdx]);

			commandList->Dispatch(dispatchX, 1, 1);
		}
	}
}

void SphSimulator::CalcSPH(ComPtr<ID3D12GraphicsCommandList> commandList)
{
	// SPH 연산
	commandList->SetPipelineState(Graphics::sphCSPSO.Get());

	// 쓰기 버퍼 상태 -> SRV -> UAV
	SetBarrier(commandList, m_particleBuffers[m_writeIdx],
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	// Hash 버퍼 UAV -> SRV
	SetUAVBarrier(commandList, m_particleBuffers[m_hashIdx]);
	SetBarrier(commandList, m_particleBuffers[m_hashIdx],
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	
	commandList->SetComputeRootDescriptorTable(0, m_particleBufferSrvGpuHandle[m_readIdx]); // SRV
	commandList->SetComputeRootDescriptorTable(1, m_particleBufferUavGpuHandle[m_writeIdx]); // UAV
	commandList->SetComputeRootDescriptorTable(2, m_constantBufferCbvGpuHandle); // CBV
	commandList->SetComputeRootDescriptorTable(3, m_particleBufferSrvGpuHandle[2]); // SRV - Hash

	const UINT groupSizeX = 256;
	UINT dispatchX = (m_maxParticles + groupSizeX - 1) / groupSizeX;
	commandList->Dispatch(dispatchX, 1, 1);
}

void SphSimulator::Render(ComPtr<ID3D12GraphicsCommandList>& commandList,
	ComPtr<ID3D12Resource>& globalConstsUploadHeap)
{
	// Sph Compute 쓰기 버퍼 -> SRV
	SetUAVBarrier(commandList, m_particleBuffers[m_writeIdx]);
	SetBarrier(commandList, m_particleBuffers[m_writeIdx],
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
	ComPtr<ID3D12Device>& device, UINT bufferIndex, UINT srvIndex, UINT uavIndex, UINT dataSize, wstring dataName)
{
	UINT particleDataSize = static_cast<UINT>(dataSize * m_maxParticles);

	auto heapPropsDefault = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	auto bufferDescParticles = CD3DX12_RESOURCE_DESC::Buffer(particleDataSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	// 버퍼 생성
	ThrowIfFailed(device->CreateCommittedResource(
		&heapPropsDefault,
		D3D12_HEAP_FLAG_NONE,
		&bufferDescParticles,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&m_particleBuffers[bufferIndex])));
	wstring bufferName = dataName + to_wstring(bufferIndex);
	m_particleBuffers[bufferIndex]->SetName(bufferName.c_str());

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = m_maxParticles;
	srvDesc.Buffer.StructureByteStride = dataSize;
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	// SRV 버퍼 설정
	CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(m_cbvSrvUavHeap->GetCPUDescriptorHandleForHeapStart(), m_cbvSrvUavSize * srvIndex);

	device->CreateShaderResourceView(
		m_particleBuffers[bufferIndex].Get(),
		&srvDesc, srvHandle
	);

	// UAV 버퍼 설정
	CD3DX12_CPU_DESCRIPTOR_HANDLE uavHandle(m_cbvSrvUavHeap->GetCPUDescriptorHandleForHeapStart(), m_cbvSrvUavSize * uavIndex);

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.FirstElement = 0;
	uavDesc.Buffer.NumElements = m_maxParticles;
	uavDesc.Buffer.StructureByteStride = dataSize;
	uavDesc.Buffer.CounterOffsetInBytes = 0;
	uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

	device->CreateUnorderedAccessView(
		m_particleBuffers[bufferIndex].Get(),
		nullptr, &uavDesc, uavHandle);
}

