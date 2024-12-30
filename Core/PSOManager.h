#pragma once

#include <d3d12.h>
#include "d3dx12.h"
#include "DirectXTex.h"
#include <D3Dcompiler.h>

#include "Helpers.h"

using Microsoft::WRL::ComPtr;

class PSOManager
{
public:
	PSOManager();
	~PSOManager();

	void Initialize(ComPtr<ID3D12Device> device);

	ComPtr<ID3D12RootSignature> m_rootSignature;
	ComPtr<ID3D12PipelineState> m_defaultPSO;
private:

};
