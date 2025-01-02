
#include "PSOManager.h"

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

		CD3DX12_DESCRIPTOR_RANGE1 srvRange;
		srvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 32, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);

		CD3DX12_ROOT_PARAMETER1 rootParameters[3];
		rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_ALL);
		rootParameters[1].InitAsConstantBufferView(1, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_ALL);
		rootParameters[2].InitAsDescriptorTable(1, &srvRange, D3D12_SHADER_VISIBILITY_PIXEL);

		// Static Sampler 설정
		D3D12_STATIC_SAMPLER_DESC sampler = {};
		sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sampler.MipLODBias = 0;
		sampler.MaxAnisotropy = 16;
		sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
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

	// DXC 초기화
	ThrowIfFailed(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils)));
	ThrowIfFailed(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler)));
	ThrowIfFailed(utils->CreateDefaultIncludeHandler(&includeHandler));
	// Create the pipeline state, which includes compiling and loading shaders.
	{

		// Vertex Shader
		ComPtr<IDxcBlobEncoding> VSSource;
		ThrowIfFailed(utils->LoadFile(L"./Shaders/BasicVS.hlsl", nullptr, &VSSource));

		DxcBuffer VSBuffer = {};
		VSBuffer.Ptr = VSSource->GetBufferPointer();
		VSBuffer.Size = VSSource->GetBufferSize();
		VSBuffer.Encoding = DXC_CP_ACP; // 기본 인코딩

		LPCWSTR VSArgs[] = {
			L"-E", L"main",       // Entry point
			L"-T", L"vs_6_0",     // Shader target (Vertex Shader, SM 6.0)
			L"-I", L"./Shaders",  // Include 경로
			L"-Zi",               // Debug 정보 포함
			L"-Od"                // 최적화 비활성화 (디버깅용)
		};

		ComPtr<IDxcResult> result1;
		ThrowIfFailed(compiler->Compile(&VSBuffer, VSArgs, _countof(VSArgs), includeHandler.Get(), IID_PPV_ARGS(&result1)));

		// 컴파일된 셰이더 가져오기
		ComPtr<IDxcBlob> vertexShader;
		result1->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&vertexShader), nullptr);

		// Pixel Shader
		ComPtr<IDxcBlobEncoding> PSSource;
		ThrowIfFailed(utils->LoadFile(L"./Shaders/BasicPS.hlsl", nullptr, &PSSource));

		DxcBuffer PSBuffer = {};
		PSBuffer.Ptr = PSSource->GetBufferPointer();
		PSBuffer.Size = PSSource->GetBufferSize();
		PSBuffer.Encoding = DXC_CP_ACP; // 기본 인코딩

		LPCWSTR PSArgs[] = {
			L"-E", L"main",       // Entry point
			L"-T", L"ps_6_0",     // Shader target (Vertex Shader, SM 6.0)
			L"-I", L"./Shaders",  // Include 경로
			L"-Zi",               // Debug 정보 포함
			L"-Od",                // 최적화 비활성화 (디버깅용)
			L"-Qstrip_debug" // Strip debug information to minimize size
		};

		ComPtr<IDxcResult> result2;
		ThrowIfFailed(compiler->Compile(&PSBuffer, PSArgs, _countof(PSArgs), includeHandler.Get(), IID_PPV_ARGS(&result2)));

		// 컴파일된 셰이더 가져오기
		ComPtr<IDxcBlob> pixelShader;
		result2->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&pixelShader), nullptr);

		// 셰이더 컴파일 에러 확인
		ComPtr<IDxcBlobUtf8> errors;
		result1->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);

		if (errors && errors->GetStringLength() > 0) {
			std::cout << "Vertex Shader Compilation Errors:\n" << errors->GetStringPointer() << std::endl;
		}

		result2->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
		if (errors && errors->GetStringLength() > 0) {
			std::cout << "Pixel Shader Compilation Errors:\n" << errors->GetStringPointer() << std::endl;
		}

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
		psoDesc.VS = { vertexShader->GetBufferPointer(), vertexShader->GetBufferSize() };
		psoDesc.PS = { pixelShader->GetBufferPointer(), pixelShader->GetBufferSize() };
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