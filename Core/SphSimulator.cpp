#include "SphSimulator.h"

SphSimulator::SphSimulator()
{
}

SphSimulator::~SphSimulator() {}

void SphSimulator::Initialize(ComPtr<ID3D12Device> device,
	ComPtr<ID3D12GraphicsCommandList> commandList, UINT width, UINT height)
{
	m_cbvSrvUavSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// 디스크립터 힙 생성
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = 5;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_heap)));

	// StructuredBuffer 생성
	CreateStructuredBuffer(device, width, height, 0);
	CreateStructuredBuffer(device, width, height, 1);

	CreateConstUploadBuffer(device, m_constsBuffer, m_constsBufferData, m_constsBufferDataBegin);
}

void SphSimulator::CreateStructuredBuffer(
	ComPtr<ID3D12Device>& device, UINT width, UINT height, UINT index)
{

	UINT particleBufferSize = m_maxParticles * sizeof(Particle);

	auto heapPropsDefault = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	auto bufferDescParticles = CD3DX12_RESOURCE_DESC::Buffer(particleBufferSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = m_particleBuffer[index]->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = m_particleBuffer[index]->GetDesc().MipLevels;

	// 버퍼 생성
	ThrowIfFailed(device->CreateCommittedResource(
		&heapPropsDefault,
		D3D12_HEAP_FLAG_NONE,
		&bufferDescParticles,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&m_particleBuffer[index])));
	wstring bufferName = L"Particle Buffer " + to_wstring(index);
	m_particleBuffer[0]->SetName(bufferName.c_str());

	// SRV 버퍼 설정
	CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(m_heap->GetCPUDescriptorHandleForHeapStart(), m_cbvSrvUavSize * index * 2);

	device->CreateShaderResourceView(
		m_particleBuffer[index].Get(),
		&srvDesc,
		srvHandle
	);

	// UAV 버퍼 설정
	CD3DX12_CPU_DESCRIPTOR_HANDLE uavHandle(m_heap->GetCPUDescriptorHandleForHeapStart(), m_cbvSrvUavSize * index * 2 + 1);

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = m_particleBuffer[index]->GetDesc().Format;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;

	device->CreateUnorderedAccessView(m_particleBuffer[index].Get(), nullptr, &uavDesc, uavHandle);
}