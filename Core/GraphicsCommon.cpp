#include "GraphicsCommon.h"

namespace Graphics
{
	ComPtr<IDxcCompiler3> compiler;
	ComPtr<IDxcUtils> utils;
	ComPtr<IDxcIncludeHandler> includeHandler;

	ComPtr<ID3D12RootSignature> rootSignature;

	ComPtr<IDxcBlob> basicVS;
	ComPtr<IDxcBlob> basicPS;

	D3D12_RASTERIZER_DESC solidRS;

	D3D12_BLEND_DESC defaultBlendDesc;

	D3D12_DEPTH_STENCIL_DESC depthStencilDesc;

	ComPtr<ID3D12PipelineState> defaultPSO;
}

void Graphics::InitDXC()
{
	// DXC �ʱ�ȭ
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

	CD3DX12_DESCRIPTOR_RANGE1 srvRange;
	srvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 24, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);

	CD3DX12_ROOT_PARAMETER1 rootParameters[4] = {};
	rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[1].InitAsConstantBufferView(1, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[2].InitAsConstantBufferView(2, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[3].InitAsDescriptorTable(1, &srvRange, D3D12_SHADER_VISIBILITY_PIXEL);

	// Static Sampler ����
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
	ThrowIfFailed(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));
}

void Graphics::InitShaders(ComPtr<ID3D12Device>& device)
{
	const wchar_t basicVSFilename[] = L"./Shaders/BasicVS.hlsl";
	CreateVertexShader(device, basicVSFilename, basicVS);

	const wchar_t basicPSFilename[] = L"./Shaders/BasicPS.hlsl";
	CreatePixelShader(device, basicPSFilename, basicPS);
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
}

void Graphics::InitBlendStates()
{
	defaultBlendDesc.AlphaToCoverageEnable = FALSE;  // ��Ƽ���ø� ���� ��� ����
	defaultBlendDesc.IndependentBlendEnable = FALSE; // ��� RenderTarget�� ������ ���� ���

	D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc = {};
	defaultRenderTargetBlendDesc.BlendEnable = FALSE; // �⺻������ ���� ��Ȱ��ȭ
	defaultRenderTargetBlendDesc.LogicOpEnable = FALSE; // �� ���� ��Ȱ��ȭ
	defaultRenderTargetBlendDesc.SrcBlend = D3D12_BLEND_ONE; // �ҽ� ���� �״�� ���
	defaultRenderTargetBlendDesc.DestBlend = D3D12_BLEND_ZERO; // ��� ���� ����
	defaultRenderTargetBlendDesc.BlendOp = D3D12_BLEND_OP_ADD; // �ҽ� + ���
	defaultRenderTargetBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE; // ���İ� �״��
	defaultRenderTargetBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO; // ��� ���İ� ����
	defaultRenderTargetBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD; // ���� �ջ�
	defaultRenderTargetBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL; // RGBA ��� ���� Ȱ��ȭ

	defaultBlendDesc.RenderTarget[0] = defaultRenderTargetBlendDesc;
}

void Graphics::InitDepthStencilStates()
{
	depthStencilDesc.DepthEnable = TRUE; // Depth �׽�Ʈ Ȱ��ȭ ����
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL; // Depth ���� ����ũ
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS; // Depth �� �Լ�
	depthStencilDesc.StencilEnable = FALSE; // ���ٽ� �׽�Ʈ Ȱ��ȭ ����
	depthStencilDesc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK; // ���ٽ� �б� ����ũ
	depthStencilDesc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK; // ���ٽ� ���� ����ũ

	// Front-facing ���ٽ� �۾� ����
	depthStencilDesc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

	// Back-facing ���ٽ� �۾� ����
	depthStencilDesc.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
}

void Graphics::InitPipelineStates(ComPtr<ID3D12Device>& device)
{

	D3D12_INPUT_ELEMENT_DESC basicIE[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};

	// Describe and create the graphcis pipeline state object (PSO).
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { basicIE, _countof(basicIE) };
	psoDesc.pRootSignature = rootSignature.Get();
	psoDesc.VS = { basicVS->GetBufferPointer(), basicVS->GetBufferSize() };
	psoDesc.PS = { basicPS->GetBufferPointer(), basicPS->GetBufferSize() };
	psoDesc.RasterizerState = solidRS;
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

	psoDesc.DepthStencilState = depthStencilDesc;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleMask = UINT_MAX;

	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&defaultPSO)));
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


void Graphics::CreateVertexShader(ComPtr<ID3D12Device>& device, const wchar_t* basicVS, ComPtr<IDxcBlob>& vertexShader)
{
	ComPtr<IDxcBlobEncoding> VSSource;
	ThrowIfFailed(utils->LoadFile(L"./Shaders/BasicVS.hlsl", nullptr, &VSSource));

	DxcBuffer VSBuffer = {};
	VSBuffer.Ptr = VSSource->GetBufferPointer();
	VSBuffer.Size = VSSource->GetBufferSize();
	VSBuffer.Encoding = DXC_CP_ACP; // �⺻ ���ڵ�

	LPCWSTR VSArgs[] = {
		L"-E", L"main",       // Entry point
		L"-T", L"vs_6_0",     // Shader target (Vertex Shader, SM 6.0)
		L"-I", L"./Shaders",  // Include ���
		L"-Zi",               // Debug ���� ����
		L"-Od",               // ����ȭ ��Ȱ��ȭ (������)
		L"-Qstrip_debug"
	};

	ComPtr<IDxcResult> result;
	ThrowIfFailed(compiler->Compile(&VSBuffer, VSArgs, _countof(VSArgs), includeHandler.Get(), IID_PPV_ARGS(&result)));

	// �����ϵ� ���̴� ��������
	result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&vertexShader), nullptr);

	// ���̴� ������ ���� Ȯ��
	ComPtr<IDxcBlobUtf8> errors;
	result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);

	if (errors && errors->GetStringLength() > 0) {
		std::cout << "Vertex Shader Compilation Errors:\n" << errors->GetStringPointer() << std::endl;
	}
}

void Graphics::CreatePixelShader(ComPtr<ID3D12Device>& device, const wchar_t* basicVS, ComPtr<IDxcBlob>& pixelShader)
{
	ComPtr<IDxcBlobEncoding> PSSource;
	ThrowIfFailed(utils->LoadFile(L"./Shaders/BasicPS.hlsl", nullptr, &PSSource));

	DxcBuffer PSBuffer = {};
	PSBuffer.Ptr = PSSource->GetBufferPointer();
	PSBuffer.Size = PSSource->GetBufferSize();
	PSBuffer.Encoding = DXC_CP_ACP; // �⺻ ���ڵ�

	LPCWSTR PSArgs[] = {
		L"-E", L"main",       // Entry point
		L"-T", L"ps_6_0",     // Shader target (Vertex Shader, SM 6.0)
		L"-I", L"./Shaders",  // Include ���
		L"-Zi",               // Debug ���� ����
		L"-Od",                // ����ȭ ��Ȱ��ȭ (������)
		L"-Qstrip_debug" // Strip debug information to minimize size
	};

	ComPtr<IDxcResult> result;
	ThrowIfFailed(compiler->Compile(&PSBuffer, PSArgs, _countof(PSArgs), includeHandler.Get(), IID_PPV_ARGS(&result)));

	// �����ϵ� ���̴� ��������
	result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&pixelShader), nullptr);

	// ���̴� ������ ���� Ȯ��
	ComPtr<IDxcBlobUtf8> errors;
	result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);

	if (errors && errors->GetStringLength() > 0) {
		std::cout << "Pixel Shader Compilation Errors:\n" << errors->GetStringPointer() << std::endl;
	}
}