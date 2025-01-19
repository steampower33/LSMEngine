#include "GraphicsCommon.h"

namespace Graphics
{
	ComPtr<IDxcCompiler3> compiler;
	ComPtr<IDxcUtils> utils;
	ComPtr<IDxcIncludeHandler> includeHandler;

	ComPtr<ID3D12RootSignature> rootSignature;

	ComPtr<IDxcBlob> basicVS;
	ComPtr<IDxcBlob> basicPS;

	ComPtr<IDxcBlob> normalVS;
	ComPtr<IDxcBlob> normalGS;
	ComPtr<IDxcBlob> normalPS;

	ComPtr<IDxcBlob> skyboxVS;
	ComPtr<IDxcBlob> skyboxPS;

	ComPtr<IDxcBlob> samplingVS;
	ComPtr<IDxcBlob> samplingPS;

	ComPtr<IDxcBlob> blurXPS;
	ComPtr<IDxcBlob> blurYPS;
	ComPtr<IDxcBlob> blurCombinePS;

	ComPtr<IDxcBlob> combineVS;
	ComPtr<IDxcBlob> combinePS;

	D3D12_RASTERIZER_DESC solidRS;
	D3D12_RASTERIZER_DESC wireRS;

	D3D12_BLEND_DESC disabledBlend;

	D3D12_DEPTH_STENCIL_DESC disabledDS;
	D3D12_DEPTH_STENCIL_DESC readWriteDS;

	ComPtr<ID3D12PipelineState> basicSolidPSO;
	ComPtr<ID3D12PipelineState> basicWirePSO;
	ComPtr<ID3D12PipelineState> normalPSO;
	ComPtr<ID3D12PipelineState> skyboxPSO;
	ComPtr<ID3D12PipelineState> samplingPSO;
	ComPtr<ID3D12PipelineState> blurXPSO;
	ComPtr<ID3D12PipelineState> blurYPSO;
	ComPtr<ID3D12PipelineState> blurCombinePSO;
	ComPtr<ID3D12PipelineState> combinePSO;
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
	textureRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 10, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);
	textureRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 10, 10, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);

	CD3DX12_DESCRIPTOR_RANGE1 filterSrvRanges[1];
	filterSrvRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1 + 3 + 3, 0, 1, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);

	CD3DX12_ROOT_PARAMETER1 rootParameters[7] = {};
	rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[1].InitAsConstantBufferView(1, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[2].InitAsConstantBufferView(2, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[3].InitAsConstantBufferView(3, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[4].InitAsDescriptorTable(2, textureRanges, D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[5].InitAsConstantBufferView(4, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[6].InitAsDescriptorTable(1, filterSrvRanges, D3D12_SHADER_VISIBILITY_PIXEL);

	// Static Sampler 설정
	D3D12_STATIC_SAMPLER_DESC sampler = {};
	sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.MipLODBias = 0.0f;
	sampler.MaxAnisotropy = 16;
	sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	sampler.MinLOD = 0.0f;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;
	sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 1, &sampler,
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
	const wchar_t basicPSFilename[] = L"./Shaders/BasicPS.hlsl";
	CreateShader(device, basicVSFilename, L"vs_6_0", basicVS);
	CreateShader(device, basicPSFilename, L"ps_6_0", basicPS);

	// Normal
	const wchar_t normalVSFilename[] = L"./Shaders/NormalVS.hlsl";
	const wchar_t normalGSFilename[] = L"./Shaders/NormalGS.hlsl";
	const wchar_t normalPSFilename[] = L"./Shaders/NormalPS.hlsl";
	CreateShader(device, normalVSFilename, L"vs_6_0", normalVS);
	CreateShader(device, normalGSFilename, L"gs_6_0", normalGS);
	CreateShader(device, normalPSFilename, L"ps_6_0", normalPS);

	// Skybox
	const wchar_t skyboxVSFilename[] = L"./Shaders/SkyboxVS.hlsl";
	const wchar_t skyboxPSFilename[] = L"./Shaders/SkyboxPS.hlsl";
	CreateShader(device, skyboxVSFilename, L"vs_6_0", skyboxVS);
	CreateShader(device, skyboxPSFilename, L"ps_6_0", skyboxPS);

	// Samplingfilter
	const wchar_t samplingVSFilename[] = L"./Shaders/SamplingVS.hlsl";
	const wchar_t samplingPSFilename[] = L"./Shaders/SamplingPS.hlsl";
	CreateShader(device, samplingVSFilename, L"vs_6_0", samplingVS);
	CreateShader(device, samplingPSFilename, L"ps_6_0", samplingPS);

	// BlurX, BlurY, BlurCombine
	const wchar_t blurXPSFilename[] = L"./Shaders/BlurXPS.hlsl";
	const wchar_t blurYPSFilename[] = L"./Shaders/BlurYPS.hlsl";
	const wchar_t blurCombineFilename[] = L"./Shaders/BlurCombinePS.hlsl";
	CreateShader(device, blurXPSFilename, L"ps_6_0", blurXPS);
	CreateShader(device, blurYPSFilename, L"ps_6_0", blurYPS);
	CreateShader(device, blurCombineFilename, L"ps_6_0", blurCombinePS);

	// CombineFilter
	const wchar_t combineVSFilename[] = L"./Shaders/CombineVS.hlsl";
	const wchar_t combinePSFilename[] = L"./Shaders/CombinePS.hlsl";
	CreateShader(device, combineVSFilename, L"vs_6_0", combineVS);
	CreateShader(device, combinePSFilename, L"ps_6_0", combinePS);
}

void Graphics::InitRasterizerStates()
{
	solidRS.FillMode = D3D12_FILL_MODE_SOLID;
	solidRS.CullMode = D3D12_CULL_MODE_NONE;
	solidRS.FrontCounterClockwise = FALSE;
	solidRS.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	solidRS.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	solidRS.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	solidRS.DepthClipEnable = TRUE;
	solidRS.MultisampleEnable = FALSE;
	solidRS.AntialiasedLineEnable = FALSE;
	solidRS.ForcedSampleCount = 0;
	solidRS.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	wireRS = solidRS;
	wireRS.FillMode = D3D12_FILL_MODE_WIREFRAME;
}

void Graphics::InitBlendStates()
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

void Graphics::InitDepthStencilStates()
{

	disabledDS.DepthEnable = FALSE;
	disabledDS.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	disabledDS.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	disabledDS.StencilEnable = FALSE;
	disabledDS.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
	disabledDS.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
	disabledDS.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	disabledDS.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	disabledDS.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	disabledDS.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	disabledDS.BackFace = disabledDS.FrontFace;

	readWriteDS = disabledDS;
	readWriteDS.DepthEnable = TRUE;
	readWriteDS.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	readWriteDS.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
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

	// Describe and create the graphcis pipeline state object (PSO).
	D3D12_GRAPHICS_PIPELINE_STATE_DESC basicSolidPSODesc = {};
	basicSolidPSODesc.InputLayout = { basicIE, _countof(basicIE) };
	basicSolidPSODesc.pRootSignature = rootSignature.Get();
	basicSolidPSODesc.VS = { basicVS->GetBufferPointer(), basicVS->GetBufferSize() };
	basicSolidPSODesc.PS = { basicPS->GetBufferPointer(), basicPS->GetBufferSize() };
	basicSolidPSODesc.RasterizerState = solidRS;
	basicSolidPSODesc.BlendState = disabledBlend;
	basicSolidPSODesc.DepthStencilState = readWriteDS;
	basicSolidPSODesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	basicSolidPSODesc.SampleDesc.Count = 1;
	basicSolidPSODesc.SampleMask = UINT_MAX;
	basicSolidPSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	basicSolidPSODesc.NumRenderTargets = 1;
	basicSolidPSODesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
	ThrowIfFailed(device->CreateGraphicsPipelineState(&basicSolidPSODesc, IID_PPV_ARGS(&basicSolidPSO)));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC basicWirePSODesc = basicSolidPSODesc;
	basicWirePSODesc.RasterizerState = wireRS;
	ThrowIfFailed(device->CreateGraphicsPipelineState(&basicWirePSODesc, IID_PPV_ARGS(&basicWirePSO)));

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
	samplingPSODesc.RTVFormats[0] = DXGI_FORMAT_R32G32B32A32_FLOAT;
	ThrowIfFailed(device->CreateGraphicsPipelineState(&samplingPSODesc, IID_PPV_ARGS(&samplingPSO)));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC blurXPSODesc = samplingPSODesc;
	blurXPSODesc.PS = { blurXPS->GetBufferPointer(), blurXPS->GetBufferSize() };
	ThrowIfFailed(device->CreateGraphicsPipelineState(&blurXPSODesc, IID_PPV_ARGS(&blurXPSO)));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC blurYPSODesc = samplingPSODesc;
	blurYPSODesc.PS = { blurYPS->GetBufferPointer(), blurYPS->GetBufferSize() };
	ThrowIfFailed(device->CreateGraphicsPipelineState(&blurYPSODesc, IID_PPV_ARGS(&blurYPSO)));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC blurCombinePSODesc = samplingPSODesc;
	blurCombinePSODesc.PS = { blurCombinePS->GetBufferPointer(), blurCombinePS->GetBufferSize() };
	ThrowIfFailed(device->CreateGraphicsPipelineState(&blurCombinePSODesc, IID_PPV_ARGS(&blurCombinePSO)));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC combinePSODesc = basicSolidPSODesc;
	combinePSODesc.VS = { combineVS->GetBufferPointer(), combineVS->GetBufferSize() };
	combinePSODesc.PS = { combinePS->GetBufferPointer(), combinePS->GetBufferSize() };
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
