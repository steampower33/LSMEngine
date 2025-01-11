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
		float width, float height, const int bloomLevels);

	~PostProcess();

	void Initialize(
		ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& commandList,
		float width, float height
	);

	void CreateTex2D(
		ComPtr<ID3D12Device>& device, ComPtr<ID3D12Resource>& texture,
		float width, float height, UINT index,
		ComPtr<ID3D12DescriptorHeap>& rtv, ComPtr<ID3D12DescriptorHeap>& srv);

	void Update(UINT m_frameIndex);

	void Render(
		ComPtr<ID3D12Device>& device,
		ComPtr<ID3D12GraphicsCommandList>& commandList,
		ComPtr<ID3D12Resource>& renderTargets,
		ComPtr<ID3D12DescriptorHeap>& rtv,
		ComPtr<ID3D12DescriptorHeap>& srv,
		ComPtr<ID3D12DescriptorHeap>& dsv,
		UINT frameIndex);

private:
	UINT m_bloomLevels;

	shared_ptr<Mesh> m_mesh;

	SamplingConstants m_samplingConsts;

	vector<shared_ptr<ImageFilter>> m_filters;
	shared_ptr<ImageFilter> m_combineFilter;

	vector<ComPtr<ID3D12Resource>> m_textures;
	ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	ComPtr<ID3D12DescriptorHeap> m_srvHeap;

	UINT rtvSize;
	UINT srvSize;
};