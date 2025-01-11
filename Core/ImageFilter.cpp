#include "ImageFilter.h"

ImageFilter::ImageFilter(
	ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& commandList,
	float width, float height, UINT index)
{
	Initialize(device, commandList, width, height, index);
}

void ImageFilter::Initialize(
	ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& commandList,
	float width, float height, UINT index)
{
	m_viewport.TopLeftX = 0.0f;
	m_viewport.TopLeftY = 0.0f;
	m_viewport.Width = width;
	m_viewport.Height = height;
	m_viewport.MinDepth = 0.0f;
	m_viewport.MaxDepth = 1.0f;

	m_samplingConstsBufferData.dx = 1.0f / width;
	m_samplingConstsBufferData.dy = 1.0f / height;

	m_samplingConstsBufferData.index = index - 1;

	CreateConstUploadBuffer(
		device, commandList,
		m_samplingConstsUploadHeap, m_samplingConstsBufferData, m_samplingConstsBufferDataBegin);
}

void ImageFilter::Update(UINT frameIndex)
{
	m_samplingConstsBufferData.index = frameIndex;
	memcpy(m_samplingConstsBufferDataBegin, &m_samplingConstsBufferData, sizeof(m_samplingConstsBufferData));
}

void ImageFilter::Render(ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	commandList->RSSetViewports(1, &m_viewport);
	commandList->SetGraphicsRootConstantBufferView(5, m_samplingConstsUploadHeap.Get()->GetGPUVirtualAddress());
}