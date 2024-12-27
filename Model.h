#pragma once

#include "Helpers.h"

namespace Renderer
{
	using namespace DirectX;

	class Model
	{
	public:
		Model(
			ComPtr<ID3D12Device> device,
			ComPtr<ID3D12GraphicsCommandList> commandList,
			ComPtr<ID3D12DescriptorHeap> heap);
		~Model();

		void Render(
			ComPtr<ID3D12Device> device,
			ComPtr<ID3D12GraphicsCommandList> commandList,
			ComPtr<ID3D12DescriptorHeap> heap);
		
		struct SceneConstantBuffer
		{
			XMFLOAT4X4 world;
			XMFLOAT4X4 view;
			XMFLOAT4X4 proj;
			float offset;
			float padding[15];
		};

		static_assert((sizeof(SceneConstantBuffer) % 256) == 0, "Constant Buffer size must be 256-byte aligned");

		void Update(const XMFLOAT4X4& world, const XMFLOAT4X4& view, const XMFLOAT4X4& proj, float offset);

	private:
		void Initialize(
			ComPtr<ID3D12Device> device, 
			ComPtr<ID3D12GraphicsCommandList> commandList,
			ComPtr<ID3D12DescriptorHeap> heap);

		int indexBufferSize;

		struct Vertex
		{
			Vertex(float x, float y, float z, float u, float v) : position(x, y, z), texcoord(u, v) {}
			XMFLOAT3 position;
			XMFLOAT2 texcoord;
		};

		ComPtr<ID3D12Resource> m_vertexBuffer;
		ComPtr<ID3D12Resource> m_vertexUploadHeap;
		D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;

		ComPtr<ID3D12Resource> m_indexBuffer;
		ComPtr<ID3D12Resource> m_indexUploadHeap;
		D3D12_INDEX_BUFFER_VIEW m_indexBufferView;

		ComPtr<ID3D12Resource> m_constantBuffer;
		ComPtr<ID3D12Resource> m_constantUploadHeap;
		SceneConstantBuffer m_constantBufferData;
		UINT8* m_pCbvDataBegin;

		ComPtr<ID3D12Resource> m_texture;
		ComPtr<ID3D12Resource> m_textureUploadHeap;
	};
}