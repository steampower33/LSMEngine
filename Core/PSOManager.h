#pragma once

#include <d3d12.h>
#include "d3dx12.h"
#include "DirectXTex.h"
#include <iostream>

#include "Helpers.h"

#include <dxcapi.h>

using Microsoft::WRL::ComPtr;

class PSOManager
{
public:
	PSOManager();
	~PSOManager();

	void Initialize(ComPtr<ID3D12Device> device);

	ComPtr<ID3D12RootSignature> m_rootSignature;
	ComPtr<ID3D12PipelineState> m_defaultPSO;

	ComPtr<IDxcCompiler3> compiler;
	ComPtr<IDxcUtils> utils;
	ComPtr<IDxcIncludeHandler> includeHandler;
};
