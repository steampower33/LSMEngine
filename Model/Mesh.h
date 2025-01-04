#pragma once

#include <d3d12.h>
#include "d3dx12.h"
#include "DirectXTex.h"
#include <vector>

#include "ConstantBuffers.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;

struct Mesh {
	ComPtr<ID3D12Resource> vertexBuffer;
	ComPtr<ID3D12Resource> vertexUploadHeap;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
	UINT vertexBufferCount = 0;

	ComPtr<ID3D12Resource> indexBuffer;
	ComPtr<ID3D12Resource> indexUploadHeap;
	D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
	UINT indexBufferCount = 0;

	ComPtr<ID3D12Resource> constsBuffer;
	ComPtr<ID3D12Resource> constsUploadHeap;
	TextureIndexConstants constsBufferData = {};
	UINT8* constsBufferDataBegin = nullptr;
};