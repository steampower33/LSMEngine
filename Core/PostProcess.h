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
		float width, float height);

	~PostProcess();

	void Initialize(
		ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& commandList,
		float width, float height
	);

	void Update(float threshold, float strength);
	void UpdateIndex(UINT frameIndex);

	void Render(
		ComPtr<ID3D12Device>& device,
		ComPtr<ID3D12GraphicsCommandList>& commandList,
		ComPtr<ID3D12Resource>& renderTargets,
		ComPtr<ID3D12DescriptorHeap>& rtv,
		ComPtr<ID3D12DescriptorHeap>& srv,
		ComPtr<ID3D12DescriptorHeap>& dsv,
		UINT frameIndex);

private:
	UINT rtvSize;
	UINT srvSize;

	UINT m_bloomLevels;
	UINT m_bufferSize;

	shared_ptr<Mesh> m_mesh;

	vector<shared_ptr<ImageFilter>> m_filters;
	vector<shared_ptr<ImageFilter>> m_blurXFilters;
	vector<shared_ptr<ImageFilter>> m_blurYFilters;
	vector<shared_ptr<ImageFilter>> m_blurCombineFilters;
	shared_ptr<ImageFilter> m_combineFilter;

	vector<ComPtr<ID3D12Resource>> m_pingPong;
	ComPtr<ID3D12DescriptorHeap> m_pingPongRtvHeap;
	ComPtr<ID3D12DescriptorHeap> m_pingPongSrvHeap;

	void CreateTex2D(
		ComPtr<ID3D12Device>& device, ComPtr<ID3D12Resource>& texture,
		UINT width, UINT height, UINT index,
		ComPtr<ID3D12DescriptorHeap>& rtvHeap, ComPtr<ID3D12DescriptorHeap>& srvHeap);

	void CreateDescriptors(ComPtr<ID3D12Device>& device);
};