#include "GraphicsCommon.h"

namespace Graphics
{
	ComPtr<IDxcCompiler3> compiler;
	ComPtr<IDxcUtils> utils;
	ComPtr<IDxcIncludeHandler> includeHandler;

	ComPtr<ID3D12RootSignature> rootSignature;
	ComPtr<ID3D12RootSignature> computeRootSignature;

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

	ComPtr<IDxcBlob> combineVS;
	ComPtr<IDxcBlob> combinePS;

	ComPtr<IDxcBlob> samplingCS;
	ComPtr<IDxcBlob> blurXCS;
	ComPtr<IDxcBlob> blurYCS;

	ComPtr<IDxcBlob> depthOnlyVS;
	ComPtr<IDxcBlob> depthOnlyPS;

	ComPtr<IDxcBlob> postEffectsVS;
	ComPtr<IDxcBlob> postEffectsPS;

	D3D12_RASTERIZER_DESC solidRS;
	D3D12_RASTERIZER_DESC wireRS;
	D3D12_RASTERIZER_DESC solidCCWRS;
	D3D12_RASTERIZER_DESC wireCCWRS;
	D3D12_RASTERIZER_DESC postProcessingRS;
	D3D12_RASTERIZER_DESC depthBiasRS;

	D3D12_BLEND_DESC disabledBlend;
	D3D12_BLEND_DESC mirrorBlend;

	D3D12_DEPTH_STENCIL_DESC basicDS;
	D3D12_DEPTH_STENCIL_DESC disabledDS;
	D3D12_DEPTH_STENCIL_DESC maskDS;
	D3D12_DEPTH_STENCIL_DESC drawMaskedDS;

	ComPtr<ID3D12PipelineState> basicSolidPSO;
	ComPtr<ID3D12PipelineState> basicWirePSO;
	ComPtr<ID3D12PipelineState> stencilMaskPSO;
	ComPtr<ID3D12PipelineState> reflectSolidPSO;
	ComPtr<ID3D12PipelineState> reflectWirePSO;
	ComPtr<ID3D12PipelineState> skyboxSolidPSO;
	ComPtr<ID3D12PipelineState> skyboxWirePSO;
	ComPtr<ID3D12PipelineState> skyboxReflectSolidPSO;
	ComPtr<ID3D12PipelineState> skyboxReflectWirePSO;
	ComPtr<ID3D12PipelineState> mirrorBlendSolidPSO;
	ComPtr<ID3D12PipelineState> mirrorBlendWirePSO;
	ComPtr<ID3D12PipelineState> sphPSO;

	ComPtr<ID3D12PipelineState> normalPSO;
	ComPtr<ID3D12PipelineState> samplingPSO;
	ComPtr<ID3D12PipelineState> combinePSO;
	ComPtr<ID3D12PipelineState> depthOnlyPSO;
	ComPtr<ID3D12PipelineState> postEffectsPSO;
	ComPtr<ID3D12PipelineState> basicSimplePSPSO;
	ComPtr<ID3D12PipelineState> shadowDepthOnlyPSO;

	ComPtr<ID3D12PipelineState> samplingCSPSO;
	ComPtr<ID3D12PipelineState> blurXCSPSO;
	ComPtr<ID3D12PipelineState> blurYCSPSO;

	UINT textureSize = 100;
	UINT cubeTextureSize = 10;
	UINT imguiTextureSize = 4;
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

	CD3DX12_DESCRIPTOR_RANGE1 textureRanges[3];
	textureRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 
		textureSize, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);
	textureRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 
		cubeTextureSize, textureSize, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);
	textureRanges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 
		imguiTextureSize, textureSize + cubeTextureSize, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);

	CD3DX12_ROOT_PARAMETER1 rootParameters[6] = {};
	rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[1].InitAsConstantBufferView(1, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[2].InitAsConstantBufferView(2, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[3].InitAsConstantBufferView(3, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[4].InitAsConstantBufferView(4, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[5].InitAsDescriptorTable(3, textureRanges, D3D12_SHADER_VISIBILITY_ALL);

	D3D12_STATIC_SAMPLER_DESC staticSamplers[4] = {};

	// LinearWrap
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].MipLODBias = 0.0f;
	staticSamplers[0].MaxAnisotropy = 1;
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	staticSamplers[0].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	staticSamplers[0].MinLOD = 0.0f;
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
	staticSamplers[0].ShaderRegister = 0;
	staticSamplers[0].RegisterSpace = 0;
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// LinearClamp
	staticSamplers[1] = staticSamplers[0];
	staticSamplers[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticSamplers[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticSamplers[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticSamplers[1].ShaderRegister = 1;

	// ShadowPoint
	staticSamplers[2] = staticSamplers[0];
	staticSamplers[2].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	staticSamplers[2].AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	staticSamplers[2].AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	staticSamplers[2].AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	staticSamplers[2].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
	staticSamplers[2].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	staticSamplers[2].ShaderRegister = 2;

	// ShadowCompare
	staticSamplers[3] = staticSamplers[2];
	staticSamplers[3].Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
	staticSamplers[3].ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	staticSamplers[3].ShaderRegister = 3;

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init_1_1(
		_countof(rootParameters),
		rootParameters,
		_countof(staticSamplers),
		staticSamplers,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error));
	ThrowIfFailed(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));
}

void Graphics::InitComputeRootSignature(ComPtr<ID3D12Device>& device)
{
	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

	if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
	{
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	CD3DX12_DESCRIPTOR_RANGE1 srvRange;
	srvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE); // 슬롯 0에 SRV

	CD3DX12_DESCRIPTOR_RANGE1 uavRange;
	uavRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE); // 슬롯 0에 UAV

	CD3DX12_ROOT_PARAMETER1 rootParameters[3] = {};
	rootParameters[0].InitAsDescriptorTable(1, &srvRange); // 입력 리소스
	rootParameters[1].InitAsDescriptorTable(1, &uavRange); // 출력 대상
	rootParameters[2].InitAsConstantBufferView(0, 0);

	D3D12_STATIC_SAMPLER_DESC staticSamplers[2] = {};

	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticSamplers[0].MipLODBias = 0.0f;
	staticSamplers[0].MaxAnisotropy = 1;
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	staticSamplers[0].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	staticSamplers[0].MinLOD = 0.0f;
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
	staticSamplers[0].ShaderRegister = 0;
	staticSamplers[0].RegisterSpace = 0;
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// LinearClamp
	staticSamplers[1] = staticSamplers[0];
	staticSamplers[1].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSamplers[0].ShaderRegister = 1;

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.Init_1_1(
		_countof(rootParameters), 
		rootParameters, 
		_countof(staticSamplers),
		staticSamplers,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error));
	ThrowIfFailed(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&computeRootSignature)));
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

	// Sampling
	const wchar_t samplingVSFilename[] = L"./Shaders/SamplingVS.hlsl";
	CreateShader(device, samplingVSFilename, L"vs_6_0", samplingVS);
	const wchar_t samplingPSFilename[] = L"./Shaders/SamplingPS.hlsl";
	CreateShader(device, samplingPSFilename, L"ps_6_0", samplingPS);
	// Combine
	const wchar_t combineVSFilename[] = L"./Shaders/CombineVS.hlsl";
	CreateShader(device, combineVSFilename, L"vs_6_0", combineVS);
	const wchar_t combinePSFilename[] = L"./Shaders/CombinePS.hlsl";
	CreateShader(device, combinePSFilename, L"ps_6_0", combinePS);

	// SamplingCS, BlurXCS, BlurYCS
	const wchar_t samplingCSFilename[] = L"./Shaders/SamplingCS.hlsl";
	CreateShader(device, samplingCSFilename, L"cs_6_0", samplingCS);
	const wchar_t blurXCSFilename[] = L"./Shaders/BlurXCS.hlsl";
	CreateShader(device, blurXCSFilename, L"cs_6_0", blurXCS);
	const wchar_t blurYCSFilename[] = L"./Shaders/BlurYCS.hlsl";
	CreateShader(device, blurYCSFilename, L"cs_6_0", blurYCS);

	// DepthOnly
	const wchar_t depthOnlyVSFilename[] = L"./Shaders/DepthOnlyVS.hlsl";
	CreateShader(device, depthOnlyVSFilename, L"vs_6_0", depthOnlyVS);
	const wchar_t depthOnlyPSFilename[] = L"./Shaders/DepthOnlyPS.hlsl";
	CreateShader(device, depthOnlyPSFilename, L"ps_6_0", depthOnlyPS);

	// PostEffects
	const wchar_t postEffectsVSFilename[] = L"./Shaders/PostEffectsVS.hlsl";
	CreateShader(device, postEffectsVSFilename, L"vs_6_0", postEffectsVS);
	const wchar_t postEffectsPSFilename[] = L"./Shaders/PostEffectsPS.hlsl";
	CreateShader(device, postEffectsPSFilename, L"ps_6_0", postEffectsPS);
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

	wireCCWRS = solidCCWRS;
	wireCCWRS.FillMode = D3D12_FILL_MODE_WIREFRAME;

	postProcessingRS = solidRS;
	postProcessingRS.CullMode = D3D12_CULL_MODE_NONE;
	postProcessingRS.DepthClipEnable = FALSE;
	postProcessingRS.MultisampleEnable = FALSE;

	depthBiasRS = solidRS;
	depthBiasRS.DepthBias = 1000;
	depthBiasRS.DepthBiasClamp = 0.0f;
	depthBiasRS.SlopeScaledDepthBias = 1.0f;
	depthBiasRS.DepthClipEnable = TRUE;
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
		D3D12_RENDER_TARGET_BLEND_DESC rtBlendDesc = {};
		rtBlendDesc.BlendEnable = TRUE;
		rtBlendDesc.LogicOpEnable = FALSE;

		// 알파 블렌드 설정 (BLEND_FACTOR 사용)
		rtBlendDesc.SrcBlend = D3D12_BLEND_BLEND_FACTOR;
		rtBlendDesc.DestBlend = D3D12_BLEND_INV_BLEND_FACTOR;
		rtBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;

		// 알파 채널 블렌드 설정
		rtBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
		rtBlendDesc.DestBlendAlpha = D3D12_BLEND_ONE;
		rtBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;

		// RGBA 전부 활성화
		rtBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		mirrorBlend.AlphaToCoverageEnable = TRUE; // MSAA 활성화
		mirrorBlend.IndependentBlendEnable = FALSE;
		mirrorBlend.RenderTarget[0] = rtBlendDesc;
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
		disabledDS = basicDS;
		disabledDS.DepthEnable = FALSE;
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

	D3D12_GRAPHICS_PIPELINE_STATE_DESC reflectWirePSODesc = basicSolidPSODesc;
	reflectWirePSODesc.RasterizerState = wireCCWRS;
	reflectWirePSODesc.DepthStencilState = drawMaskedDS;
	ThrowIfFailed(device->CreateGraphicsPipelineState(&reflectWirePSODesc, IID_PPV_ARGS(&reflectWirePSO)));

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

	D3D12_GRAPHICS_PIPELINE_STATE_DESC skyboxSolidPSODesc = basicSolidPSODesc;
	skyboxSolidPSODesc.VS = { skyboxVS->GetBufferPointer(), skyboxVS->GetBufferSize() };
	skyboxSolidPSODesc.PS = { skyboxPS->GetBufferPointer(), skyboxPS->GetBufferSize() };
	ThrowIfFailed(device->CreateGraphicsPipelineState(&skyboxSolidPSODesc, IID_PPV_ARGS(&skyboxSolidPSO)));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC skyboxWirePSODesc = skyboxSolidPSODesc;
	skyboxWirePSODesc.RasterizerState = wireRS;
	ThrowIfFailed(device->CreateGraphicsPipelineState(&skyboxWirePSODesc, IID_PPV_ARGS(&skyboxWirePSO)));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC skyboxReflectSolidPSODesc = skyboxSolidPSODesc;
	skyboxReflectSolidPSODesc.RasterizerState = solidCCWRS;
	skyboxReflectSolidPSODesc.DepthStencilState = drawMaskedDS;
	ThrowIfFailed(device->CreateGraphicsPipelineState(&skyboxReflectSolidPSODesc, IID_PPV_ARGS(&skyboxReflectSolidPSO)));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC skyboxReflectWirePSODesc = skyboxReflectSolidPSODesc;
	skyboxReflectWirePSODesc.RasterizerState = wireCCWRS;
	skyboxReflectWirePSODesc.DepthStencilState = drawMaskedDS;
	ThrowIfFailed(device->CreateGraphicsPipelineState(&skyboxReflectWirePSODesc, IID_PPV_ARGS(&skyboxReflectWirePSO)));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC samplingPSODesc = basicSolidPSODesc;
	samplingPSODesc.InputLayout = { nullptr, 0 };
	samplingPSODesc.VS = { samplingVS->GetBufferPointer(), samplingVS->GetBufferSize() };
	samplingPSODesc.PS = { samplingPS->GetBufferPointer(), samplingPS->GetBufferSize() };
	samplingPSODesc.RasterizerState = postProcessingRS;
	samplingPSODesc.SampleDesc.Count = 1;
	samplingPSODesc.DepthStencilState = disabledDS;
	ThrowIfFailed(device->CreateGraphicsPipelineState(&samplingPSODesc, IID_PPV_ARGS(&samplingPSO)));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC combinePSODesc = samplingPSODesc;
	combinePSODesc.VS = { combineVS->GetBufferPointer(), combineVS->GetBufferSize() };
	combinePSODesc.PS = { combinePS->GetBufferPointer(), combinePS->GetBufferSize() };
	combinePSODesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
	ThrowIfFailed(device->CreateGraphicsPipelineState(&combinePSODesc, IID_PPV_ARGS(&combinePSO)));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC depthOnlyPSODesc = basicSolidPSODesc;
	depthOnlyPSODesc.VS = { depthOnlyVS->GetBufferPointer(), depthOnlyVS->GetBufferSize() };
	depthOnlyPSODesc.PS = { depthOnlyPS->GetBufferPointer(), depthOnlyPS->GetBufferSize() };
	depthOnlyPSODesc.SampleDesc.Count = 1;
	depthOnlyPSODesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	ThrowIfFailed(device->CreateGraphicsPipelineState(&depthOnlyPSODesc, IID_PPV_ARGS(&depthOnlyPSO)));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC shadowDepthOnlyPSODesc = basicSolidPSODesc;
	shadowDepthOnlyPSODesc.VS = { depthOnlyVS->GetBufferPointer(), depthOnlyVS->GetBufferSize() };
	shadowDepthOnlyPSODesc.PS = { depthOnlyPS->GetBufferPointer(), depthOnlyPS->GetBufferSize() };
	shadowDepthOnlyPSODesc.SampleDesc.Count = 1;
	shadowDepthOnlyPSODesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	shadowDepthOnlyPSODesc.RasterizerState = depthBiasRS;
	ThrowIfFailed(device->CreateGraphicsPipelineState(&shadowDepthOnlyPSODesc, IID_PPV_ARGS(&shadowDepthOnlyPSO)));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC postEffectsPSODesc = basicSolidPSODesc;
	postEffectsPSODesc.VS = { postEffectsVS->GetBufferPointer(), postEffectsVS->GetBufferSize() };
	postEffectsPSODesc.PS = { postEffectsPS->GetBufferPointer(), postEffectsPS->GetBufferSize() };
	postEffectsPSODesc.RasterizerState = postProcessingRS;
	postEffectsPSODesc.SampleDesc.Count = 1;
	ThrowIfFailed(device->CreateGraphicsPipelineState(&postEffectsPSODesc, IID_PPV_ARGS(&postEffectsPSO)));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC basicSimplePSPSODesc = basicSolidPSODesc;
	basicSimplePSPSODesc.PS = { simplePS->GetBufferPointer(), simplePS->GetBufferSize() };
	ThrowIfFailed(device->CreateGraphicsPipelineState(&basicSimplePSPSODesc, IID_PPV_ARGS(&basicSimplePSPSO)));

	D3D12_COMPUTE_PIPELINE_STATE_DESC samplingCSPSODesc = {};
	samplingCSPSODesc.pRootSignature = computeRootSignature.Get();
	samplingCSPSODesc.CS = { samplingCS->GetBufferPointer(), samplingCS->GetBufferSize() };
	samplingCSPSODesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	ThrowIfFailed(device->CreateComputePipelineState(&samplingCSPSODesc, IID_PPV_ARGS(&samplingCSPSO)));

	D3D12_COMPUTE_PIPELINE_STATE_DESC blurXCSPSODesc = samplingCSPSODesc;
	blurXCSPSODesc.CS = { blurXCS->GetBufferPointer(), blurXCS->GetBufferSize() };
	ThrowIfFailed(device->CreateComputePipelineState(&blurXCSPSODesc, IID_PPV_ARGS(&blurXCSPSO)));

	D3D12_COMPUTE_PIPELINE_STATE_DESC blurYCSPSODesc = samplingCSPSODesc;
	blurYCSPSODesc.CS = { blurYCS->GetBufferPointer(), blurYCS->GetBufferSize() };
	ThrowIfFailed(device->CreateComputePipelineState(&blurYCSPSODesc, IID_PPV_ARGS(&blurYCSPSO)));
}

void Graphics::Initialize(ComPtr<ID3D12Device>& device)
{
	InitDXC();
	InitRootSignature(device);
	InitComputeRootSignature(device);
	InitShaders(device);
	InitRasterizerStates();
	InitBlendStates();
	InitDepthStencilStates();
	InitPipelineStates(device);
}


void Graphics::CreateShader(
	ComPtr<ID3D12Device>& device,
	const wchar_t* filename,
	const wchar_t* targetProfile,
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
	std::wstring shaderIncludePath = std::filesystem::absolute(L"./Shaders").wstring();

	std::vector<LPCWSTR> args;

#if defined(_DEBUG)
	args = {
		L"-E", L"main",
		L"-T", targetProfile,
		L"-I", shaderIncludePath.c_str(),
		L"-Zi",
		L"-Od",
		L"-Fd", pdbFilename.c_str()
	};
#else
	args = {
		L"-E", L"main",
		L"-T", targetProfile,
		L"-I", shaderIncludePath.c_str(),
		L"-O2"
	};
#endif

	ComPtr<IDxcResult> result;
	ThrowIfFailed(compiler->Compile(&buffer, args.data(), static_cast<UINT32>(args.size()), includeHandler.Get(), IID_PPV_ARGS(&result)));

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
