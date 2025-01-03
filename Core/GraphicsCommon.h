#pragma once

#include <d3d12.h>
#include "d3dx12.h"
#include "DirectXTex.h"
#include <iostream>

#include "Helpers.h"

#include <dxcapi.h>

using Microsoft::WRL::ComPtr;

// 참고: DirectX_Graphic-Samples 미니엔진
// https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/MiniEngine/Core/GraphicsCommon.h

namespace Graphics
{
	extern ComPtr<IDxcCompiler3> compiler;
	extern ComPtr<IDxcUtils> utils;
	extern ComPtr<IDxcIncludeHandler> includeHandler;

	extern ComPtr<ID3D12RootSignature> rootSignature;

	extern ComPtr<IDxcBlob> basicVS;
	extern ComPtr<IDxcBlob> basicPS;

	extern D3D12_RASTERIZER_DESC solidRS;

	extern D3D12_BLEND_DESC defaultBlendDesc;

	extern D3D12_DEPTH_STENCIL_DESC depthStencilDesc;

	extern ComPtr<ID3D12PipelineState> defaultPSO;
	
	void Initialize(ComPtr<ID3D12Device>& device);
	void InitDXC();
	void InitRootSignature(ComPtr<ID3D12Device>& device);
	void InitShaders(ComPtr<ID3D12Device>& device);
	void InitRasterizerStates();
	void InitBlendStates();
	void InitDepthStencilStates();
	void InitPipelineStates(ComPtr<ID3D12Device>& device);

	void CreateVertexShader(ComPtr<ID3D12Device>& device, const wchar_t* basicVS, ComPtr<IDxcBlob>& vertexShader);
	void CreatePixelShader(ComPtr<ID3D12Device>& device, const wchar_t* basicVS, ComPtr<IDxcBlob>& pixelShader);
}