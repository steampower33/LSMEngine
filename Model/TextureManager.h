#pragma once

#include <iostream>
#include <d3d12.h>
#include "d3dx12.h"
#include <dxgi1_6.h>
#include <unordered_map>
#include <string>

#include "Mesh.h"
#include "MeshData.h"
#include "Helpers.h"
#include "ConstantBuffers.h"
#include "GraphicsCommon.h"
#include "DescriptorHeapAllocator.h"

using namespace std;
using Microsoft::WRL::ComPtr;

class TextureManager
{
public:
	TextureManager(
		ComPtr<ID3D12Device>& device,
		HeapAllocator& srvAlloc);

	~TextureManager() {}

	void Initialize(
		ComPtr<ID3D12Device>& device,
		ComPtr<ID3D12GraphicsCommandList>& commandList);

	void LoadTextures(
		ComPtr<ID3D12Device>& device,
		ComPtr<ID3D12GraphicsCommandList>& commandList,
		ComPtr<ID3D12CommandQueue>& commandQueue,
		const MeshData& meshData,
		shared_ptr<Mesh>& newMesh,
		CubemapIndexConstants& cubemapIndexConstsBufferData);

	void CreateMipMapTexture(
		ComPtr<ID3D12Device>& device,
		ComPtr<ID3D12GraphicsCommandList>& commandList,
		string filename,
		shared_ptr<Mesh>& newMesh,
		CubemapIndexConstants& cubemapIndexConstsBufferData);

	void CreateDDSTexture(
		ComPtr<ID3D12Device>& device,
		ComPtr<ID3D12GraphicsCommandList>& commandList,
		ComPtr<ID3D12CommandQueue>& commandQueue,
		string filename,
		shared_ptr<Mesh>& newMesh,
		CubemapIndexConstants& cubemapIndexConstsBufferData);

	UINT m_textureCnt = 0;
	UINT m_cubeTextureCnt = 50;

	vector<ComPtr<ID3D12Resource>> m_textures;
	vector<ComPtr<ID3D12Resource>> m_texturesUploadHeap;

	ComPtr<ID3D12DescriptorHeap> m_textureHeap;

	CD3DX12_CPU_DESCRIPTOR_HANDLE m_heapStartCpu = {};

	unordered_map<string, TextureInfo> m_textureIdx;

private:
	bool CheckDuplcateFilename(
		unordered_map<string, TextureInfo>& textureIdx,
		const string& filename,
		const string& lowerFilename,
		shared_ptr<Mesh>& newMesh,
		CubemapIndexConstants& cubemapIndexConstsBufferData);
};