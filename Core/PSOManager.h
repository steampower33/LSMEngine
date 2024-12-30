#pragma once

#include "Helpers.h"

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
