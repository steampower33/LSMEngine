#pragma once

#include <d3d12.h>
#include "d3dx12.h"
#include "ConstantBuffers.h"
#include "Helpers.h"
#include "GraphicsCommon.h"

using Microsoft::WRL::ComPtr;

enum FilterOption {
	COPY,
	BLOOM_DOWN,
	BLOOM_UP,
	BLUR_X,
	BLUR_Y,
	COMBINE,
};

class ImageFilter
{
public:
	ImageFilter(
		ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& commandList,
		UINT width, UINT height, UINT index, FilterOption option, UINT bloomLevels);
	~ImageFilter() {}

	void Update(SamplingConstants& m_combineConsts);

	void Render(
		ComPtr<ID3D12GraphicsCommandList>& commandList,
		ComPtr<ID3D12DescriptorHeap>& rtv,
		ComPtr<ID3D12DescriptorHeap>& srv);

	void ComputeRender(
		ComPtr<ID3D12GraphicsCommandList>& commandList,
		ComPtr<ID3D12DescriptorHeap>& heaps);

private:
	UINT cbvSrvUavSize;
	FilterOption m_option;
	UINT srvIndex;
	UINT uavIndex;
	UINT m_width;
	UINT m_height;
	UINT tgx;
	UINT tgy;
	SamplingConstants m_samplingConstsBufferData;
	ComPtr<ID3D12Resource> m_samplingConstsUploadHeap;
	UINT8* m_samplingConstsBufferDataBegin;

};

