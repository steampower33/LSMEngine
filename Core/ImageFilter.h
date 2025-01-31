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
		UINT width, UINT height, UINT index);
	~ImageFilter() {}

	void Update(SamplingConstants& m_combineConsts);

	void Render(
		ComPtr<ID3D12GraphicsCommandList>& commandList,
		ComPtr<ID3D12DescriptorHeap>& rtvHeap,
		UINT rtvOffset,
		ComPtr<ID3D12DescriptorHeap>& dsvHeap,
		UINT indexBufferCount);

private:
	void Initialize(
		ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& commandList,
		UINT width, UINT height, UINT index);

	CD3DX12_VIEWPORT m_viewport;

	SamplingConstants m_samplingConstsBufferData;
	ComPtr<ID3D12Resource> m_samplingConstsUploadHeap;
	UINT8* m_samplingConstsBufferDataBegin;

};

