#include "GraphicsCommon.h"

namespace Graphics
{
	ComPtr<IDxcCompiler3> compiler;
	ComPtr<IDxcUtils> utils;
	ComPtr<IDxcIncludeHandler> includeHandler;

	ComPtr<ID3D12RootSignature> rootSignature;

	ComPtr<IDxcBlob> basicVS;
	ComPtr<IDxcBlob> basicPS;
	ComPtr<IDxcBlob> simplePS;

	ComPtr<IDxcBlob> normalVS;
	ComPtr<IDxcBlob> normalGS;
	ComPtr<IDxcBlob> normalPS;

	ComPtr<IDxcBlob> skyboxVS;
	ComPtr<IDxcBlob> skyboxPS;

	ComPtr<IDxcBlob> samplingVS;
	ComPtr<IDxcBlob> samplingPS;

	ComPtr<IDxcBlob> bloomDownPS;
	ComPtr<IDxcBlob> bloomUpPS;

	ComPtr<IDxcBlob> combineVS;
	ComPtr<IDxcBlob> combinePS;

	D3D12_RASTERIZER_DESC solidRS;
	D3D12_RASTERIZER_DESC wireRS;
	D3D12_RASTERIZER_DESC solidCCWRS;
	D3D12_RASTERIZER_DESC wireCCWRS;
	D3D12_RASTERIZER_DESC postProcessingRS;

	D3D12_BLEND_DESC disabledBlend;
	D3D12_BLEND_DESC mirrorBlend;

	D3D12_DEPTH_STENCIL_DESC basicDS;
	D3D12_DEPTH_STENCIL_DESC maskDS;
	D3D12_DEPTH_STENCIL_DESC drawMaskedDS;

	ComPtr<ID3D12PipelineState> basicSolidPSO;
	ComPtr<ID3D12PipelineState> basicWirePSO;
	ComPtr<ID3D12PipelineState> stencilMaskPSO;
	ComPtr<ID3D12PipelineState> reflectSolidPSO;
	ComPtr<ID3D12PipelineState> reflectWirePSO;
	ComPtr<ID3D12PipelineState> mirrorBlendSolidPSO;
	ComPtr<ID3D12PipelineState> mirrorBlendWirePSO;

	ComPtr<ID3D12PipelineState> normalPSO;
	ComPtr<ID3D12PipelineState> skyboxPSO;
	ComPtr<ID3D12PipelineState> samplingPSO;
	ComPtr<ID3D12PipelineState> bloomDownPSO;
	ComPtr<ID3D12PipelineState> bloomUpPSO;
	ComPtr<ID3D12PipelineState> combinePSO;

	UINT bloomLevels;
	UINT textureSize = 20;
	UINT cubeTextureSize = 10;
}

void Graphics::InitDXC()
{
	// DXC 초기화
	ThrowIfFailed(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils)));
	ThrowIfFailed(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler)));
	ThrowIfFailed(utils->CreateDefaultIncludeHandler(&includeHandler));
}

void Graphics::InitRootSignature(ComPtr<ID3D12Device>& device)
{
	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

	if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
	{
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	CD3DX12_DESCRIPTOR_RANGE1 textureRanges[2];
	textureRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 50, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);
	textureRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 10, 50, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);

	bloomLevels = 3;
	CD3DX12_DESCRIPTOR_RANGE1 filterSrvRanges[1];
	filterSrvRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1 + bloomLevels, 0, 1, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);

	CD3DX12_ROOT_PARAMETER1 rootParameters[7] = {};
	rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[1].InitAsConstantBufferView(1, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[2].InitAsConstantBufferView(2, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[3].InitAsConstantBufferView(3, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[4].InitAsDescriptorTable(2, textureRanges, D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[5].InitAsConstantBufferView(4, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[6].InitAsDescriptorTable(1, filterSrvRanges, D3D12_SHADER_VISIBILITY_PIXEL);

	// 디스크립터 힙 생성
	D3D12_DESCRIPTOR_HEAP_DESC samplerHeapDesc = {};
	samplerHeapDesc.NumDescriptors = 2; // 샘플러 개수
	samplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
	samplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE; // 셰이더에서 접근 가능
	samplerHeapDesc.NodeMask = 0;

	ComPtr<ID3D12DescriptorHeap> samplerHeap;
	HRESULT hr = device->CreateDescriptorHeap(&samplerHeapDesc, IID_PPV_ARGS(&samplerHeap));
	if (FAILED(hr)) {
		throw std::runtime_error("Failed to create sampler descriptor heap.");
	}

	// 샘플러 상태 정의
	D3D12_STATIC_SAMPLER_DESC staticSamplers[2] = {};

	// 첫 번째 정적 샘플러 (Wrap 모드)
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR; // 필터링 방식
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP; // U축 주소 모드
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP; // V축 주소 모드
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP; // W축 주소 모드
	staticSamplers[0].MipLODBias = 0.0f;
	staticSamplers[0].MaxAnisotropy = 1;
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	staticSamplers[0].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	staticSamplers[0].MinLOD = 0.0f;
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
	staticSamplers[0].ShaderRegister = 0; // s0에서 참조
	staticSamplers[0].RegisterSpace = 0;
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// 두 번째 정적 샘플러 (Clamp 모드)
	staticSamplers[1] = staticSamplers[0]; // 기본 값 복사
	staticSamplers[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP; // U축 주소 모드 변경
	staticSamplers[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP; // V축 주소 모드 변경
	staticSamplers[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP; // W축 주소 모드 변경
	staticSamplers[1].ShaderRegister = 1; // s1에서 참조

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init_1_1(
		_countof(rootParameters),  // 루트 매개변수 개수
		rootParameters,            // 루트 매개변수 배열
		_countof(staticSamplers),  // 정적 샘플러 개수
		staticSamplers,            // 정적 샘플러 배열
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error));
	ThrowIfFailed(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));
}

void Graphics::InitShaders(ComPtr<ID3D12Device>& device)
{
	// Basic
	const wchar_t basicVSFilename[] = L"./Shaders/BasicVS.hlsl";
	CreateShader(device, basicVSFilename, L"vs_6_0", basicVS);
	const wchar_t basicPSFilename[] = L"./Shaders/BasicPS.hlsl";
	CreateShader(device, basicPSFilename, L"ps_6_0", basicPS);
	const wchar_t simplePSFilename[] = L"./Shaders/SimplePS.hlsl";
	CreateShader(device, simplePSFilename, L"ps_6_0", simplePS);

	// Normal
	const wchar_t normalVSFilename[] = L"./Shaders/NormalVS.hlsl";
	CreateShader(device, normalVSFilename, L"vs_6_0", normalVS);
	const wchar_t normalGSFilename[] = L"./Shaders/NormalGS.hlsl";
	CreateShader(device, normalGSFilename, L"gs_6_0", normalGS);
	const wchar_t normalPSFilename[] = L"./Shaders/NormalPS.hlsl";
	CreateShader(device, normalPSFilename, L"ps_6_0", normalPS);

	// Skybox
	const wchar_t skyboxVSFilename[] = L"./Shaders/SkyboxVS.hlsl";
	CreateShader(device, skyboxVSFilename, L"vs_6_0", skyboxVS);
	const wchar_t skyboxPSFilename[] = L"./Shaders/SkyboxPS.hlsl";
	CreateShader(device, skyboxPSFilename, L"ps_6_0", skyboxPS);

	// Samplingfilter
	const wchar_t samplingVSFilename[] = L"./Shaders/SamplingVS.hlsl";
	CreateShader(device, samplingVSFilename, L"vs_6_0", samplingVS);
	const wchar_t samplingPSFilename[] = L"./Shaders/SamplingPS.hlsl";
	CreateShader(device, samplingPSFilename, L"ps_6_0", samplingPS);

	// BloomDown, BloomUp
	const wchar_t bloomDownPSFilename[] = L"./Shaders/BloomDownPS.hlsl";
	CreateShader(device, bloomDownPSFilename, L"ps_6_0", bloomDownPS);
	const wchar_t bloomUpPSFilename[] = L"./Shaders/BloomUpPS.hlsl";
	CreateShader(device, bloomUpPSFilename, L"ps_6_0", bloomUpPS);

	// CombineFilter
	const wchar_t combineVSFilename[] = L"./Shaders/CombineVS.hlsl";
	CreateShader(device, combineVSFilename, L"vs_6_0", combineVS);
	const wchar_t combinePSFilename[] = L"./Shaders/CombinePS.hlsl";
	CreateShader(device, combinePSFilename, L"ps_6_0", combinePS);
}

void Graphics::InitRasterizerStates()
{
	solidRS.FillMode = D3D12_FILL_MODE_SOLID;
	solidRS.CullMode = D3D12_CULL_MODE_BACK;
	solidRS.FrontCounterClockwise = FALSE;
	solidRS.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	solidRS.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	solidRS.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	solidRS.DepthClipEnable = TRUE;
	solidRS.MultisampleEnable = TRUE;
	solidRS.AntialiasedLineEnable = FALSE;
	solidRS.ForcedSampleCount = 0;
	solidRS.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	wireRS = solidRS;
	wireRS.FillMode = D3D12_FILL_MODE_WIREFRAME;

	solidCCWRS = solidRS;
	solidCCWRS.FrontCounterClockwise = TRUE;

	postProcessingRS = solidRS;
	postProcessingRS.DepthClipEnable = FALSE;
}

void Graphics::InitBlendStates()
{
	{
		disabledBlend.AlphaToCoverageEnable = FALSE;  // 멀티샘플링 알파 사용 여부
		disabledBlend.IndependentBlendEnable = FALSE; // 모든 RenderTarget이 동일한 설정 사용

		D3D12_RENDER_TARGET_BLEND_DESC disabledBlendDesc = {};
		disabledBlendDesc.BlendEnable = FALSE; // 기본적으로 블렌딩 비활성화
		disabledBlendDesc.LogicOpEnable = FALSE; // 논리 연산 비활성화
		disabledBlendDesc.SrcBlend = D3D12_BLEND_ONE; // 소스 색상 그대로 사용
		disabledBlendDesc.DestBlend = D3D12_BLEND_ZERO; // 대상 색상 무시
		disabledBlendDesc.BlendOp = D3D12_BLEND_OP_ADD; // 소스 + 대상
		disabledBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE; // 알파값 그대로
		disabledBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO; // 대상 알파값 무시
		disabledBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD; // 알파 합산
		disabledBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL; // RGBA 모두 쓰기 활성화

		disabledBlend.RenderTarget[0] = disabledBlendDesc;
	}

	{
		D3D12_BLEND_DESC mirrorBlend = {};
		mirrorBlend.AlphaToCoverageEnable = FALSE;
		mirrorBlend.IndependentBlendEnable = FALSE;

		D3D12_RENDER_TARGET_BLEND_DESC mirrorBlendDesc = {};
		mirrorBlendDesc.BlendEnable = TRUE;
		mirrorBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
		mirrorBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		mirrorBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
		mirrorBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
		mirrorBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
		mirrorBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
		mirrorBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		// 소스 알파 값을 반사 강도로 설정
		// 이는 픽셀 셰이더에서 설정된 알파 값을 사용해야 함
		mirrorBlend.RenderTarget[0] = mirrorBlendDesc;
	}
}

void Graphics::InitDepthStencilStates()
{
	{
		basicDS.DepthEnable = TRUE;
		basicDS.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		basicDS.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		basicDS.StencilEnable = FALSE;
		basicDS.StencilReadMask = 0xFF;
		basicDS.StencilWriteMask = 0xFF;

		// FrontFace 설정
		basicDS.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		basicDS.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		basicDS.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
		basicDS.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

		basicDS.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		basicDS.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		basicDS.BackFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;
		basicDS.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	}

	{
		maskDS = basicDS;
		maskDS.DepthEnable = TRUE;
		maskDS.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		maskDS.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		maskDS.StencilEnable = TRUE;
		maskDS.StencilReadMask = 0xFF;
		maskDS.StencilWriteMask = 0xFF;

		// FrontFace 설정
		maskDS.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		maskDS.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		maskDS.FrontFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;
		maskDS.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	}

	{
		drawMaskedDS = maskDS;
		drawMaskedDS.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		drawMaskedDS.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

		drawMaskedDS.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		drawMaskedDS.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		drawMaskedDS.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
		drawMaskedDS.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;
	}
}

void Graphics::InitPipelineStates(ComPtr<ID3D12Device>& device)
{

	D3D12_INPUT_ELEMENT_DESC basicIE[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};

	UINT sampleCount = 4;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC basicSolidPSODesc = {};
	basicSolidPSODesc.InputLayout = { basicIE, _countof(basicIE) };
	basicSolidPSODesc.pRootSignature = rootSignature.Get();
	basicSolidPSODesc.VS = { basicVS->GetBufferPointer(), basicVS->GetBufferSize() };
	basicSolidPSODesc.PS = { basicPS->GetBufferPointer(), basicPS->GetBufferSize() };
	basicSolidPSODesc.RasterizerState = solidRS;
	basicSolidPSODesc.BlendState = disabledBlend;
	basicSolidPSODesc.DepthStencilState = basicDS;
	basicSolidPSODesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	basicSolidPSODesc.SampleDesc.Count = sampleCount;
	basicSolidPSODesc.SampleDesc.Quality = 0;
	basicSolidPSODesc.SampleMask = UINT_MAX;
	basicSolidPSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	basicSolidPSODesc.NumRenderTargets = 1;
	basicSolidPSODesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
	ThrowIfFailed(device->CreateGraphicsPipelineState(&basicSolidPSODesc, IID_PPV_ARGS(&basicSolidPSO)));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC basicWirePSODesc = basicSolidPSODesc;
	basicWirePSODesc.RasterizerState = wireRS;
	ThrowIfFailed(device->CreateGraphicsPipelineState(&basicWirePSODesc, IID_PPV_ARGS(&basicWirePSO)));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC stencilMaskPSODesc = basicSolidPSODesc;
	stencilMaskPSODesc.DepthStencilState = maskDS;
	stencilMaskPSODesc.PS = { simplePS->GetBufferPointer(), simplePS->GetBufferSize() };
	ThrowIfFailed(device->CreateGraphicsPipelineState(&stencilMaskPSODesc, IID_PPV_ARGS(&stencilMaskPSO)));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC reflectSolidPSODesc = basicSolidPSODesc;
	reflectSolidPSODesc.RasterizerState = solidCCWRS;
	reflectSolidPSODesc.DepthStencilState = drawMaskedDS;
	ThrowIfFailed(device->CreateGraphicsPipelineState(&reflectSolidPSODesc, IID_PPV_ARGS(&reflectSolidPSO)));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC mirrorBlendSolidPSODesc = basicSolidPSODesc;
	mirrorBlendSolidPSODesc.BlendState = mirrorBlend;
	mirrorBlendSolidPSODesc.DepthStencilState = drawMaskedDS;
	ThrowIfFailed(device->CreateGraphicsPipelineState(&mirrorBlendSolidPSODesc, IID_PPV_ARGS(&mirrorBlendSolidPSO)));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC normalPSODesc = basicSolidPSODesc;
	normalPSODesc.VS = { normalVS->GetBufferPointer(), normalVS->GetBufferSize() };
	normalPSODesc.GS = { normalGS->GetBufferPointer(), normalGS->GetBufferSize() };
	normalPSODesc.PS = { normalPS->GetBufferPointer(), normalPS->GetBufferSize() };
	normalPSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	ThrowIfFailed(device->CreateGraphicsPipelineState(&normalPSODesc, IID_PPV_ARGS(&normalPSO)));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC skyboxPSODesc = basicSolidPSODesc;
	skyboxPSODesc.VS = { skyboxVS->GetBufferPointer(), skyboxVS->GetBufferSize() };
	skyboxPSODesc.PS = { skyboxPS->GetBufferPointer(), skyboxPS->GetBufferSize() };
	ThrowIfFailed(device->CreateGraphicsPipelineState(&skyboxPSODesc, IID_PPV_ARGS(&skyboxPSO)));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC samplingPSODesc = basicSolidPSODesc;
	samplingPSODesc.VS = { samplingVS->GetBufferPointer(), samplingVS->GetBufferSize() };
	samplingPSODesc.PS = { samplingPS->GetBufferPointer(), samplingPS->GetBufferSize() };
	samplingPSODesc.RasterizerState = postProcessingRS;
	samplingPSODesc.SampleDesc.Count = 1;
	ThrowIfFailed(device->CreateGraphicsPipelineState(&samplingPSODesc, IID_PPV_ARGS(&samplingPSO)));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC bloomDownPSODesc = samplingPSODesc;
	bloomDownPSODesc.PS = { bloomDownPS->GetBufferPointer(), bloomDownPS->GetBufferSize() };
	ThrowIfFailed(device->CreateGraphicsPipelineState(&bloomDownPSODesc, IID_PPV_ARGS(&bloomDownPSO)));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC bloomUpPSODesc = samplingPSODesc;
	bloomUpPSODesc.PS = { bloomUpPS->GetBufferPointer(), bloomUpPS->GetBufferSize() };
	ThrowIfFailed(device->CreateGraphicsPipelineState(&bloomUpPSODesc, IID_PPV_ARGS(&bloomUpPSO)));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC combinePSODesc = samplingPSODesc;
	combinePSODesc.VS = { combineVS->GetBufferPointer(), combineVS->GetBufferSize() };
	combinePSODesc.PS = { combinePS->GetBufferPointer(), combinePS->GetBufferSize() };
	combinePSODesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	ThrowIfFailed(device->CreateGraphicsPipelineState(&combinePSODesc, IID_PPV_ARGS(&combinePSO)));

}

void Graphics::Initialize(ComPtr<ID3D12Device>& device)
{
	InitDXC();
	InitRootSignature(device);
	InitShaders(device);
	InitRasterizerStates();
	InitBlendStates();
	InitDepthStencilStates();
	InitPipelineStates(device);
}


void Graphics::CreateShader(
	ComPtr<ID3D12Device>& device,
	const wchar_t* filename,
	const wchar_t* targetProfile, // Shader Target Profile
	ComPtr<IDxcBlob>& shaderBlob)
{
	ComPtr<IDxcBlobEncoding> source;
	ThrowIfFailed(utils->LoadFile(filename, nullptr, &source));

	DxcBuffer buffer = {};
	buffer.Ptr = source->GetBufferPointer();
	buffer.Size = source->GetBufferSize();
	buffer.Encoding = DXC_CP_ACP; // 기본 인코딩

	std::filesystem::path filepath(filename);
	std::wstring shaderName = filepath.stem().wstring(); // 확장자 제거된 파일 이름

	//std::wcout << L"Shader Name: " << shaderName << std::endl;

	std::wstring pdbFilename = L"./PDB/" + std::wstring(shaderName) + L".pdb";
	LPCWSTR args[] = {
		L"-E", L"main",							// Entry point
		L"-T", targetProfile,					// Shader target
		L"-I", L"./Shaders",					// Include 경로
		L"-Zi",									// Debug 정보 포함
		L"-Od",									// 최적화 비활성화 (디버깅용)
		L"-Fd", pdbFilename.c_str(),			// PDB 파일 생성 경로
	};

	ComPtr<IDxcResult> result;
	ThrowIfFailed(compiler->Compile(&buffer, args, _countof(args), includeHandler.Get(), IID_PPV_ARGS(&result)));

	// 컴파일된 셰이더 가져오기
	result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);

	// PDB 파일 출력
	ComPtr<IDxcBlob> pdbBlob;
	result->GetOutput(DXC_OUT_PDB, IID_PPV_ARGS(&pdbBlob), nullptr);
	if (pdbBlob) {
		std::ofstream pdbFile(pdbFilename, std::ios::binary);
		pdbFile.write((const char*)pdbBlob->GetBufferPointer(), pdbBlob->GetBufferSize());
		pdbFile.close();
	}

	// 셰이더 컴파일 에러 확인
	ComPtr<IDxcBlobUtf8> errors;
	result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);

	if (errors && errors->GetStringLength() > 0) {
		std::wcout << targetProfile;
		std::cout << " Compilation Errors:\n" << errors->GetStringPointer() << std::endl;
	}
}
