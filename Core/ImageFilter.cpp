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

	scissorRect.left = 0;
	scissorRect.top = 0;
	scissorRect.right = static_cast<LONG>(width);
	scissorRect.bottom = static_cast<LONG>(height);

	m_samplingConstsBufferData.dx = 1.0f / width;
	m_samplingConstsBufferData.dy = 1.0f / height;

	m_samplingConstsBufferData.threshold = 0.0f;
	m_samplingConstsBufferData.strength = 0.0f;

	m_samplingConstsBufferData.index = index;

	CreateConstUploadBuffer(
		device, commandList,
		m_samplingConstsUploadHeap, m_samplingConstsBufferData, m_samplingConstsBufferDataBegin);
}

void ImageFilter::Update(float threshold, float strength)
{
	m_samplingConstsBufferData.threshold = threshold;
	m_samplingConstsBufferData.strength = strength;
	memcpy(m_samplingConstsBufferDataBegin, &m_samplingConstsBufferData, sizeof(m_samplingConstsBufferData));
}

void ImageFilter::UpdateIndex(UINT frameIndex)
{
	m_samplingConstsBufferData.index = frameIndex;
	memcpy(m_samplingConstsBufferDataBegin, &m_samplingConstsBufferData, sizeof(m_samplingConstsBufferData));
}

void ImageFilter::Render(ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	commandList->RSSetViewports(1, &m_viewport);
	commandList->RSSetScissorRects(1, &scissorRect);
	commandList->SetGraphicsRootConstantBufferView(5, m_samplingConstsUploadHeap.Get()->GetGPUVirtualAddress());
}