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
	heapDesc.NumDescriptors = 5;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_heap)));
	m_heap->SetName(L"SPH SRV/UAV/CBV Heap");

	// StructuredBuffer 생성
	CreateStructuredBuffer(device, width, height, 0);
	CreateStructuredBuffer(device, width, height, 1);

	// 업로드 버퍼 생성 및 데이터 복사
	UploadAndCopyParticleData(device, commandList);

	CreateConstUploadBuffer(device, m_constantBuffer, m_constantBufferData, m_constantBufferDataBegin);

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = m_constantBuffer->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = m_constantBufferSize;

	CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHandle(m_heap->GetCPUDescriptorHandleForHeapStart(), m_cbvSrvUavSize * 4);
	device->CreateConstantBufferView(
		&cbvDesc,
		cbvHandle
	);

	CD3DX12_GPU_DESCRIPTOR_HANDLE currentGpuHandle(m_heap->GetGPUDescriptorHandleForHeapStart());

	m_particleBufferSrvGpuHandle[0] = currentGpuHandle;
	currentGpuHandle.Offset(1, m_cbvSrvUavSize);
	m_particleBufferUavGpuHandle[0] = currentGpuHandle;
	currentGpuHandle.Offset(1, m_cbvSrvUavSize);
	m_particleBufferSrvGpuHandle[1] = currentGpuHandle;
	currentGpuHandle.Offset(1, m_cbvSrvUavSize);
	m_particleBufferUavGpuHandle[1] = currentGpuHandle;
	currentGpuHandle.Offset(1, m_cbvSrvUavSize);
	m_constantBufferCbvGpuHandle = currentGpuHandle;
}

void SphSimulator::GenerateParticles()
{
	m_particlesCPU.resize(m_maxParticles);

	vector<XMFLOAT4> rainbow = {
		{1.0f, 0.0f, 0.0f, 0.0f},  // Red
		{1.0f, 0.65f, 0.0f, 0.0f}, // Orange
		{1.0f, 1.0f, 0.0f, 0.0f},  // Yellow
		{0.0f, 1.0f, 0.0f, 0.0f},  // Green
		{0.0f, 0.0f, 1.0f, 0.0f},  // Blue
		{0.3f, 0.0f, 0.5f, 0.0f},  // Indigo
		{0.5f, 0.0f, 1.0f, 0.0f}   // Violet/Purple
	};

	random_device rd;
	mt19937 gen(rd());
	uniform_real_distribution<float> dp(-1.0f, 1.0f);
	uniform_int_distribution<size_t> dc(0, rainbow.size() - 1);

	for (auto& p : m_particlesCPU) {
		p.position = XMFLOAT2(dp(gen), dp(gen));
		p.color = rainbow[dc(gen)];
		p.size = 1.0f;
		p.life = 5.0f;
	}
}

void SphSimulator::UploadAndCopyParticleData(ComPtr<ID3D12Device> device,
	ComPtr<ID3D12GraphicsCommandList> commandList)
{
	UINT particleDataSize = static_cast<UINT>(m_particlesCPU.size() * sizeof(Particle));
	auto heapPropsUpload = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto bufferDescUpload = CD3DX12_RESOURCE_DESC::Buffer(particleDataSize);
	ThrowIfFailed(device->CreateCommittedResource(
		&heapPropsUpload,
		D3D12_HEAP_FLAG_NONE,
		&bufferDescUpload,
		D3D12_RESOURCE_STATE_GENERIC_READ, // UPLOAD 힙은 GENERIC_READ에서 시작
		nullptr,
		IID_PPV_ARGS(&m_uploadBuffer)));
	m_uploadBuffer->SetName(L"Particle Upload Buffer");

	D3D12_SUBRESOURCE_DATA particleData = {};
	particleData.pData = m_particlesCPU.data();
	particleData.RowPitch = particleDataSize;
	particleData.SlicePitch = particleDataSize;

	// 대상 버퍼(Buffer 0) 상태 전이: COMMON -> COPY_DEST
	SetBarrier(commandList, m_particleBuffer[0],
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

	UpdateSubresources(commandList.Get(), m_particleBuffer[0].Get(), m_uploadBuffer.Get(), 0, 0, 1, &particleData);

	// 대상 버퍼(Buffer 0) 상태 전이: COPY_DEST -> UAV
	SetBarrier(commandList, m_particleBuffer[0],
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	// Buffer 1 첫 사용 상태도 여기서 설정 - COMMON -> SRV
	SetBarrier(commandList, m_particleBuffer[1],
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
}

void SphSimulator::Update(float dt)
{
	// 일단은 CBV 전체를 업데이트
	m_constantBufferData.deltaTime = dt;
	memcpy(m_constantBufferDataBegin, &m_constantBufferData, sizeof(m_constantBufferData));

	// 읽기 쓰기 인덱스 갱신
	m_readIdx = !m_readIdx;
	m_writeIdx = !m_writeIdx;
}

void SphSimulator::Render(ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	commandList->SetComputeRootSignature(Graphics::sphComputeRootSignature.Get());
	commandList->SetPipelineState(Graphics::sphCSPSO.Get());

	// m_particleBuffer[readIdx] -> UAV -> SRV
	// SRV로 사용하기전 UAV 쓰기 작업 기다림
	SetUAVBarrier(commandList, m_particleBuffer[m_readIdx]);
	SetBarrier(commandList, m_particleBuffer[m_readIdx],
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	// m_particleBuffer[writeIdx] -> SRV -> UAV
	SetBarrier(commandList, m_particleBuffer[m_writeIdx],
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	ID3D12DescriptorHeap* ppHeap[] = { m_heap.Get() };
	commandList->SetDescriptorHeaps(_countof(ppHeap), ppHeap);

	commandList->SetComputeRootDescriptorTable(0, m_particleBufferSrvGpuHandle[m_readIdx]); // SRV
	commandList->SetComputeRootDescriptorTable(1, m_particleBufferUavGpuHandle[m_writeIdx]); // UAV
	commandList->SetComputeRootDescriptorTable(2, m_constantBufferCbvGpuHandle); // CBV

	const UINT groupSizeX = 64;
	UINT dispatchX = (m_maxParticles + groupSizeX - 1) / groupSizeX;
	commandList->Dispatch(dispatchX, 1, 1);
}

void SphSimulator::CreateStructuredBuffer(
	ComPtr<ID3D12Device>& device, UINT width, UINT height, UINT index)
{
	UINT particleDataSize = static_cast<UINT>(m_particlesCPU.size() * sizeof(Particle));

	auto heapPropsDefault = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	auto bufferDescParticles = CD3DX12_RESOURCE_DESC::Buffer(particleDataSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	// 버퍼 생성
	ThrowIfFailed(device->CreateCommittedResource(
		&heapPropsDefault,
		D3D12_HEAP_FLAG_NONE,
		&bufferDescParticles,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&m_particleBuffer[index])));
	wstring bufferName = L"Particle Buffer " + to_wstring(index);
	m_particleBuffer[index]->SetName(bufferName.c_str());

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = m_maxParticles;
	srvDesc.Buffer.StructureByteStride = sizeof(Particle);
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	// SRV 버퍼 설정
	CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(m_heap->GetCPUDescriptorHandleForHeapStart(), m_cbvSrvUavSize * index * 2);

	device->CreateShaderResourceView(
		m_particleBuffer[index].Get(),
		&srvDesc, srvHandle
	);

	// UAV 버퍼 설정
	CD3DX12_CPU_DESCRIPTOR_HANDLE uavHandle(m_heap->GetCPUDescriptorHandleForHeapStart(), m_cbvSrvUavSize * (index * 2 + 1));

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.FirstElement = 0;
	uavDesc.Buffer.NumElements = m_maxParticles;
	uavDesc.Buffer.StructureByteStride = sizeof(Particle);
	uavDesc.Buffer.CounterOffsetInBytes = 0;
	uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

	device->CreateUnorderedAccessView(
		m_particleBuffer[index].Get(),
		nullptr, &uavDesc, uavHandle);
}