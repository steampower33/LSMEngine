#pragma once

#include <d3d12.h>
#include "d3dx12.h"
#include "DirectXTex.h"

#include "MeshData.h"
#include "ConstantBuffers.h"
#include "Helpers.h"
#include "Mesh.h"
#include "GraphicsCommon.h"
#include "TextureManager.h"

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
		const vector<MeshData>& meshDatas,
		CubemapIndexConstants& cubemapIndexConstsBufferData,
		shared_ptr<TextureManager>& textureManager);

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

	void OnlyCallConstsMemcpy();

	XMFLOAT4 pos = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	string key;

	vector<shared_ptr<Mesh>> m_meshes;
	ComPtr<ID3D12Resource> m_meshConstsUploadHeap;
	MeshConstants m_meshConstsBufferData;
	UINT8* m_meshConstsBufferDataBegin;

private:
	void Initialize(
		ComPtr<ID3D12Device>& device,
		ComPtr<ID3D12GraphicsCommandList>& commandList,
		ComPtr<ID3D12CommandQueue>& commandQueue,
		const vector<MeshData>& meshDatas,
		CubemapIndexConstants& cubemapIndexConstsBufferData,
		shared_ptr<TextureManager>& textureManager);


};