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

using namespace std;
using Microsoft::WRL::ComPtr;

class TextureManager
{
public:
	TextureManager(
		ComPtr<ID3D12Device>& device,
		ComPtr<ID3D12GraphicsCommandList>& commandList,
		ComPtr<ID3D12DescriptorHeap>& textureHeap);
	~TextureManager() {}

	void LoadTextures(
		ComPtr<ID3D12Device>& device,
		ComPtr<ID3D12GraphicsCommandList>& commandList,
		ComPtr<ID3D12CommandQueue>& commandQueue,
		const MeshData& meshData,
		shared_ptr<Mesh>& newMesh,
		CubemapIndexConstants& cubemapIndexConstsBufferData);

private:
	vector<ComPtr<ID3D12Resource>> m_textures;
	vector<ComPtr<ID3D12Resource>> m_texturesUploadHeap;

	CD3DX12_CPU_DESCRIPTOR_HANDLE m_heapStartCpu = {};
	
	unordered_map<string, int> m_textureIdx;
	UINT m_normaltextureCnt = 0;
	UINT m_cubeTextureCnt = 10;

private:
	bool CheckDuplcateFilename(
		unordered_map<string, int>& textureIdx,
		const string& filename,
		shared_ptr<Mesh>& newMesh);
};