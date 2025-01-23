#include "ImageFilter.h"

ImageFilter::ImageFilter(
	ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& commandList,
	UINT width, UINT height, UINT index)
{
	Initialize(device, commandList, width, height, index);
}

void ImageFilter::Initialize(
	ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& commandList,
	UINT width, UINT height, UINT index)
{
	m_viewport.TopLeftX = 0.0f;
	m_viewport.TopLeftY = 0.0f;
	m_viewport.Width = static_cast<float>(width);
	m_viewport.Height = static_cast<float>(height);
	m_viewport.MinDepth = 0.0f;
	m_viewport.MaxDepth = 1.0f;

	m_samplingConstsBufferData.dx = 1.0f / width;
	m_samplingConstsBufferData.dy = 1.0f / height;

	m_samplingConstsBufferData.index = index;

	CreateConstUploadBuffer(
		device, commandList,
		m_samplingConstsUploadHeap, m_samplingConstsBufferData, m_samplingConstsBufferDataBegin);
}

void ImageFilter::Update(SamplingConstants& m_combineConsts)
{
	m_samplingConstsBufferData.strength = m_combineConsts.strength;
	m_samplingConstsBufferData.exposure = m_combineConsts.exposure;
	m_samplingConstsBufferData.gamma = m_combineConsts.gamma;
	memcpy(m_samplingConstsBufferDataBegin, &m_samplingConstsBufferData, sizeof(m_samplingConstsBufferData));
}

void ImageFilter::Render(
	ComPtr<ID3D12GraphicsCommandList>& commandList,
	ComPtr<ID3D12DescriptorHeap>& rtvHeap,
	UINT rtvOffset,
	ComPtr<ID3D12DescriptorHeap>& dsvHeap,
	ComPtr<ID3D12DescriptorHeap>& srvHeap,
	UINT indexBufferCount)
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart(), rtvOffset);
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(dsvHeap->GetCPUDescriptorHandleForHeapStart());
	commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

	const float color[] = { 0.0f, 0.2f, 1.0f, 1.0f };
	commandList->ClearRenderTargetView(rtvHandle, color, 0, nullptr);
	commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	commandList->RSSetViewports(1, &m_viewport);
	commandList->SetGraphicsRootConstantBufferView(4, m_samplingConstsUploadHeap.Get()->GetGPUVirtualAddress());

	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->DrawIndexedInstanced(indexBufferCount, 1, 0, 0, 0);
}