#pragma once

#include <d3d12.h>
#include "d3dx12.h"
#include "DirectXTex.h"

#include "MeshData.h"
#include "ConstantBuffers.h"
#include "Helpers.h"
#include "Mesh.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;

class Model
{
public:
	Model(
		ComPtr<ID3D12Device>& device,
		ComPtr<ID3D12GraphicsCommandList>& commandList,
		CD3DX12_CPU_DESCRIPTOR_HANDLE textureHandle,
		const std::vector<MeshData>& meshDatas);
	~Model();

	void Render(
		ComPtr<ID3D12Device>& device,
		ComPtr<ID3D12GraphicsCommandList>& commandList,
		ComPtr<ID3D12DescriptorHeap>& textureHeap);

	void Update();

private:
	void Initialize(
		ComPtr<ID3D12Device>& device,
		ComPtr<ID3D12GraphicsCommandList>& commandList,
		CD3DX12_CPU_DESCRIPTOR_HANDLE textureHandle,
		const std::vector<MeshData>& meshDatas);

	std::unordered_set<std::string> filenames;

	std::vector<std::shared_ptr<Mesh>> m_meshes;
	
	ComPtr<ID3D12Resource> m_meshConstsUploadHeap;
	MeshConstants m_meshConstsBufferData;
	UINT8 *m_meshConstsBufferDataBegin;

	std::vector<ComPtr<ID3D12Resource>> textures;
	std::vector<ComPtr<ID3D12Resource>> texturesUploadHeap;
	std::vector<UINT> texturesIdx;
};