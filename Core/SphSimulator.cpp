#include "SphSimulator.h"

SphSimulator::SphSimulator()
{
}

SphSimulator::~SphSimulator() {}

void SphSimulator::Initialize(ComPtr<ID3D12Device> device,
	ComPtr<ID3D12GraphicsCommandList> commandList)
{
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = 5;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_srvUavHeap)));

}