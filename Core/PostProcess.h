#pragma once

#include <d3d12.h>
#include "d3dx12.h"

#include "GeometryGenerator.h"
#include "Mesh.h"
#include "MeshData.h"
#include "Helpers.h"
#include "ImageFilter.h"
#include "ConstantBuffers.h"
#include "GraphicsCommon.h"

class PostProcess
{
public:
	PostProcess(
		ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& commandList,
		float width, float height, UINT frameIndex);

	~PostProcess();

	void Initialize(
		ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& commandList,
		float width, float height
	);

	void Update(SamplingConstants& m_combineConsts);

	void Render(
		ComPtr<ID3D12Device>& device,
		ComPtr<ID3D12GraphicsCommandList>& commandList,
		ComPtr<ID3D12Resource>& renderTargets,
		ComPtr<ID3D12DescriptorHeap>& rtv,
		ComPtr<ID3D12DescriptorHeap>& srv,
		ComPtr<ID3D12DescriptorHeap>& dsv,
		UINT frameIndex);

private:
	UINT m_frameIndex;
	UINT m_bloomLevels;
	UINT m_bufferSize;

	UINT rtvSize;
	UINT srvSize;

	shared_ptr<Mesh> m_mesh;

	shared_ptr<ImageFilter> m_copyFilter;
	vector<shared_ptr<ImageFilter>> m_bloomDownFilters;
	vector<shared_ptr<ImageFilter>> m_bloomUpFilters;
	shared_ptr<ImageFilter> m_combineFilter;

	vector<ComPtr<ID3D12Resource>> m_buffer;
	ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	ComPtr<ID3D12DescriptorHeap> m_srvHeap;
	
	ComPtr<ID3D12Resource> m_dsBuffer;
	ComPtr<ID3D12DescriptorHeap> m_dsvHeap;

	void CreateTex2D(
		ComPtr<ID3D12Device>& device, ComPtr<ID3D12Resource>& texture,
		UINT width, UINT height, UINT index,
		ComPtr<ID3D12DescriptorHeap>& rtvHeap, ComPtr<ID3D12DescriptorHeap>& srvHeap);

	void CreateDescriptors(ComPtr<ID3D12Device>& device, float width, float height);
};