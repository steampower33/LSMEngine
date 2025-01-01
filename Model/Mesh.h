#pragma once

#include <d3d12.h>
#include "d3dx12.h"
#include "DirectXTex.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;

struct Mesh {
	ComPtr<ID3D12Resource> vertexBuffer;
	ComPtr<ID3D12Resource> vertexUploadHeap;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;

	ComPtr<ID3D12Resource> indexBuffer;
	ComPtr<ID3D12Resource> indexUploadHeap;
	D3D12_INDEX_BUFFER_VIEW indexBufferView;
	int indexBufferCount;

	ComPtr<ID3D12Resource> texture[8];
	ComPtr<ID3D12Resource> textureUploadHeap[8];
	int textureCnt;
	int startIdx;
};