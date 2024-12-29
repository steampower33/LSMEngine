
#include "PSOManager.h"

namespace Renderer
{
	PSOManager::PSOManager() {}

	PSOManager::~PSOManager() {}

	void PSOManager::Initialize(ComPtr<ID3D12Device> device)
	{
		// Create an empty root signature
		{
			D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

			featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

			if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
			{
				featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
			}

			CD3DX12_DESCRIPTOR_RANGE1 ranges[2];
			ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 2, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_NONE);
			ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

			CD3DX12_ROOT_PARAMETER1 rootParameters[2];
			rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_VERTEX);
			rootParameters[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_PIXEL);

			D3D12_STATIC_SAMPLER_DESC sampler = {};
			sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
			sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			sampler.MipLODBias = 0;
			sampler.MaxAnisotropy = 0;
			sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
			sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
			sampler.MinLOD = 0.0f;
			sampler.MaxLOD = D3D12_FLOAT32_MAX;
			sampler.ShaderRegister = 0;
			sampler.RegisterSpace = 0;
			sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

			CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
			rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 1, &sampler, 
				D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

			ComPtr<ID3DBlob> signature;
			ComPtr<ID3DBlob> error;
			ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error));
			ThrowIfFailed(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
		}

		// Create the pipeline state, which includes compiling and loading shaders.
		{
			ComPtr<ID3DBlob> vertexShader;
			ComPtr<ID3DBlob> pixelShader;

			ThrowIfFailed(D3DCompileFromFile(
				L"BasicVS.hlsl",
				nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "vs_5_0", 0, 0, &vertexShader, nullptr));
			ThrowIfFailed(D3DCompileFromFile(
				L"BasicPS.hlsl",
				nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "ps_5_0", 0, 0, &pixelShader, nullptr));

			D3D12_INPUT_ELEMENT_DESC basicIE[] =
			{
				{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
				{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
				{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
			};

			// Default rasterizer states
			D3D12_RASTERIZER_DESC RasterizerDefault;
			RasterizerDefault.FillMode = D3D12_FILL_MODE_SOLID;
			RasterizerDefault.CullMode = D3D12_CULL_MODE_NONE;
			RasterizerDefault.FrontCounterClockwise = FALSE;
			RasterizerDefault.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
			RasterizerDefault.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
			RasterizerDefault.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
			RasterizerDefault.DepthClipEnable = TRUE;
			RasterizerDefault.MultisampleEnable = FALSE;
			RasterizerDefault.AntialiasedLineEnable = FALSE;
			RasterizerDefault.ForcedSampleCount = 0;
			RasterizerDefault.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

			D3D12_DEPTH_STENCIL_DESC depthStencilDesc = {};
			depthStencilDesc.DepthEnable = TRUE; // Depth 테스트 활성화 여부
			depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL; // Depth 쓰기 마스크
			depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS; // Depth 비교 함수
			depthStencilDesc.StencilEnable = FALSE; // 스텐실 테스트 활성화 여부
			depthStencilDesc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK; // 스텐실 읽기 마스크
			depthStencilDesc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK; // 스텐실 쓰기 마스크

			// Front-facing 스텐실 작업 설정
			depthStencilDesc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
			depthStencilDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
			depthStencilDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
			depthStencilDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

			// Back-facing 스텐실 작업 설정
			depthStencilDesc.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
			depthStencilDesc.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
			depthStencilDesc.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
			depthStencilDesc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

			// Describe and create the graphcis pipeline state object (PSO).
			D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
			psoDesc.InputLayout = { basicIE, _countof(basicIE) };
			psoDesc.pRootSignature = m_rootSignature.Get();
			psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
			psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
			psoDesc.RasterizerState = RasterizerDefault;
			psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

			psoDesc.DepthStencilState = depthStencilDesc;
			psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
			psoDesc.SampleDesc.Count = 1;
			psoDesc.SampleMask = UINT_MAX;

			psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			psoDesc.NumRenderTargets = 1;
			psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
			ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_defaultPSO)));
		}
	}
}