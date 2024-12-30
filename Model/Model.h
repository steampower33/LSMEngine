#pragma once

#include "Helpers.h"
#include "Vertex.h"

using namespace DirectX;

class Model
{
public:
	Model(
		ComPtr<ID3D12Device> device,
		ComPtr<ID3D12GraphicsCommandList> commandList,
		CD3DX12_CPU_DESCRIPTOR_HANDLE basicHandle);
	~Model();

	void Render(
		ComPtr<ID3D12Device> device,
		ComPtr<ID3D12GraphicsCommandList> commandList
	);

	void Update();

private:
	void Initialize(
		ComPtr<ID3D12Device> device,
		ComPtr<ID3D12GraphicsCommandList> commandList,
		CD3DX12_CPU_DESCRIPTOR_HANDLE basicHandle);

	int indexBufferSize;

	ComPtr<ID3D12Resource> m_vertexBuffer;
	ComPtr<ID3D12Resource> m_vertexUploadHeap;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;

	ComPtr<ID3D12Resource> m_indexBuffer;
	ComPtr<ID3D12Resource> m_indexUploadHeap;
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView;

	ComPtr<ID3D12Resource> m_meshConstsUploadHeap;
	MeshConstants m_meshConstsBufferData;
	UINT8* m_meshConstsBufferDataBegin;

	ComPtr<ID3D12Resource> m_texture;
	ComPtr<ID3D12Resource> m_textureUploadHeap;
};