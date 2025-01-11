#pragma once

#include <d3d12.h>
#include "d3dx12.h"
#include "ConstantBuffers.h"
#include "Helpers.h"

using Microsoft::WRL::ComPtr;

class ImageFilter
{
public:
	ImageFilter(
		ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& commandList,
		float width, float height, UINT index);
	~ImageFilter() {}

	void Update(UINT frameIndex);
	void Render(ComPtr<ID3D12GraphicsCommandList>& commandList);

private:
	void Initialize(
		ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& commandList,
		float width, float height, UINT index);

	CD3DX12_VIEWPORT m_viewport;

	SamplingConstants m_samplingConstsBufferData;
	ComPtr<ID3D12Resource> m_samplingConstsUploadHeap;
	UINT8* m_samplingConstsBufferDataBegin;
};

