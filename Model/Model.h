#pragma once

#include <d3d12.h>
#include "d3dx12.h"
#include "DirectXTex.h"

#include "MeshData.h"
#include "ConstantBuffers.h"
#include "Helpers.h"
#include "Mesh.h"
#include "GraphicsCommon.h"

using namespace std;
using Microsoft::WRL::ComPtr;
using namespace DirectX;

class Model
{
public:
	Model(
		ComPtr<ID3D12Device>& device,
		ComPtr<ID3D12GraphicsCommandList>& commandList,
		ComPtr<ID3D12CommandQueue>& commandQueue,
		CD3DX12_CPU_DESCRIPTOR_HANDLE textureHandle,
		const vector<MeshData>& meshDatas,
		UINT& totalTextureCnt,
		unordered_map<string, int>& textureIdx);

	~Model();

	void Render(
		ComPtr<ID3D12Device>& device,
		ComPtr<ID3D12GraphicsCommandList>& commandList,
		ComPtr<ID3D12DescriptorHeap>& textureHeap,
		GuiState& guiState);

	void RenderSkybox(
		ComPtr<ID3D12Device>& device,
		ComPtr<ID3D12GraphicsCommandList>& commandList,
		ComPtr<ID3D12DescriptorHeap>& textureHeap,
		GuiState& guiState);

	void Update();

	XMFLOAT4 pos = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);

private:
	void Initialize(
		ComPtr<ID3D12Device>& device,
		ComPtr<ID3D12GraphicsCommandList>& commandList,
		ComPtr<ID3D12CommandQueue>& commandQueue,
		CD3DX12_CPU_DESCRIPTOR_HANDLE textureHandle,
		const vector<MeshData>& meshDatas,
		UINT& totalTextureCnt,
		unordered_map<string, int>& textureIdx);

	bool CheckDuplcateFilename(
		unordered_map<string, int>& textureIdx,
		const string& filename,
		shared_ptr<Mesh>& newMesh);

	vector<shared_ptr<Mesh>> m_meshes;

	ComPtr<ID3D12Resource> m_meshConstsUploadHeap;
	MeshConstants m_meshConstsBufferData;
	UINT8* m_meshConstsBufferDataBegin;

	vector<ComPtr<ID3D12Resource>> textures;
	vector<ComPtr<ID3D12Resource>> texturesUploadHeap;

};