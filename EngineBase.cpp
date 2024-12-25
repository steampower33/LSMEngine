#include "EngineBase.h"

#include "WinApp.h"

using namespace WindowApplication;

class WinApp;

namespace EngineCore
{
	HeapAllocator EngineBase::m_srvAlloc;

	EngineBase::EngineBase(UINT width, UINT height, std::wstring name) :
		m_width(width), m_height(height),
		m_frameIndex(0),
		m_sceneSize(800, 600),
		m_viewport(0.0f, 0.0f, static_cast<float>(m_sceneSize.x), static_cast<float>(m_sceneSize.y)),
		m_scissorRect(0.0f, 0.0f, static_cast<LONG>(width), static_cast<LONG>(height)),
		m_rtvDescriptorSize(0),
		m_windowVisible(true),
		m_windowedMode(true),
		m_pCbvDataBegin(nullptr),
		m_aspectRatio(static_cast<float>(m_sceneSize.x) / static_cast<float>(m_sceneSize.y)),
		m_useWarpDevice(false)
	{

	}

	EngineBase::~EngineBase()
	{
	}

	void EngineBase::Init()
	{
		LoadPipeline();
		LoadAssets();
		LoadGUI();
	}

	void EngineBase::LoadPipeline()
	{
		ThrowIfFailed(CoInitializeEx(nullptr, COINIT_MULTITHREADED));

		UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
		// Enable the debug layer (requires the Graphics Tools "optional feature").
		// NOTE: Enabling the debug layer after device creation will invalidate the active device.
		{
			ComPtr<ID3D12Debug> debugController;
			if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
			{
				debugController->EnableDebugLayer();

				// Enable additional debug layers.
				dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
			}
		}
#endif

		ComPtr<IDXGIFactory4> factory;
		ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));

		if (m_useWarpDevice)
		{
			ComPtr<IDXGIAdapter> warpAdapter;
			ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

			ThrowIfFailed(D3D12CreateDevice(
				warpAdapter.Get(),
				D3D_FEATURE_LEVEL_11_0,
				IID_PPV_ARGS(&m_device)
			));
		}
		else
		{
			ComPtr<IDXGIAdapter1> hardwareAdapter;
			GetHardwareAdapter(factory.Get(), &hardwareAdapter);

			ThrowIfFailed(D3D12CreateDevice(
				hardwareAdapter.Get(),
				D3D_FEATURE_LEVEL_11_0,
				IID_PPV_ARGS(&m_device)
			));
		}

		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

		ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));

		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.BufferCount = FrameCount;
		swapChainDesc.Width = m_width;
		swapChainDesc.Height = m_height;
		swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.SampleDesc.Count = 1;

		ComPtr<IDXGISwapChain1> swapChain;
		ThrowIfFailed(factory->CreateSwapChainForHwnd(
			m_commandQueue.Get(),        // Swap chain needs the queue so that it can force a flush on it.
			WindowApplication::WinApp::m_hwnd,
			&swapChainDesc,
			nullptr,
			nullptr,
			&swapChain
		));

		// This sample does not support fullscreen transitions.
		ThrowIfFailed(factory->MakeWindowAssociation(WindowApplication::WinApp::m_hwnd, DXGI_MWA_NO_ALT_ENTER));

		ThrowIfFailed(swapChain.As(&m_swapChain));
		m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

		// Create descriptor heaps.
		{
			D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
			rtvHeapDesc.NumDescriptors = FrameCount;
			rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));

			m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

			D3D12_DESCRIPTOR_HEAP_DESC m_viewRTVHeapDesc = {};
			m_viewRTVHeapDesc.NumDescriptors = FrameCount;
			m_viewRTVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			m_viewRTVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			ThrowIfFailed(m_device->CreateDescriptorHeap(&m_viewRTVHeapDesc, IID_PPV_ARGS(&m_sceneRTVHeap)));

			D3D12_DESCRIPTOR_HEAP_DESC m_viewSRVHeapDesc = {};
			m_viewSRVHeapDesc.NumDescriptors = FrameCount;
			m_viewSRVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			m_viewSRVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			ThrowIfFailed(m_device->CreateDescriptorHeap(&m_viewSRVHeapDesc, IID_PPV_ARGS(&m_sceneSRVHeap)));

			D3D12_DESCRIPTOR_HEAP_DESC basicHeapDesc = {};
			basicHeapDesc.NumDescriptors = 1;
			basicHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			basicHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			ThrowIfFailed(m_device->CreateDescriptorHeap(&basicHeapDesc, IID_PPV_ARGS(&m_basicHeap)));

			D3D12_DESCRIPTOR_HEAP_DESC imguiHeapDesc = {};
			imguiHeapDesc.NumDescriptors = 32;
			imguiHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			imguiHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			ThrowIfFailed(m_device->CreateDescriptorHeap(&imguiHeapDesc, IID_PPV_ARGS(&m_imguiHeap)));
			m_srvAlloc.Create(m_device.Get(), m_imguiHeap.Get());
		}

		{
			CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

			// Create a RTV for each frame
			for (UINT n = 0; n < FrameCount; n++)
			{
				ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n])));
				m_device->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);
				rtvHandle.Offset(1, m_rtvDescriptorSize);
			}
		}

		{

			for (UINT n = 0; n < FrameCount; n++)
			{
				ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator[n])));
			}

			// Create the command list.
			ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator[m_frameIndex].Get(), m_pipelineState.Get(), IID_PPV_ARGS(&m_commandList)));

			for (UINT n = 0; n < FrameCount; n++)
			{
				ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence[n])));
				m_fenceValue[n] = 0;
			}

			// Create an event handle to use for frame synchronization.
			m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
			if (m_fenceEvent == nullptr)
			{
				ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
			}
		}
	}

	void EngineBase::LoadAssets()
	{
		// Create an empty root signature
		{
			D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

			featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

			if (FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
			{
				featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
			}

			CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
			ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

			CD3DX12_ROOT_PARAMETER1 rootParameters[1];
			rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);

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
			rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

			ComPtr<ID3DBlob> signature;
			ComPtr<ID3DBlob> error;
			ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error));
			ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
		}

		// Create the pipeline state, which includes compiling and loading shaders.
		{
			ComPtr<ID3DBlob> vertexShader;
			ComPtr<ID3DBlob> pixelShader;

			// 컴파일 플래그 설정
			UINT compileFlags = 0;
#if defined(_DEBUG) || defined(DEBUG)
			compileFlags |= D3DCOMPILE_DEBUG; // 디버그 정보를 포함
			compileFlags |= D3DCOMPILE_SKIP_OPTIMIZATION; // 최적화 비활성화
#endif

			ThrowIfFailed(D3DCompileFromFile(
				L"VertexShader.hlsl",
				nullptr, nullptr, "main", "vs_5_0", compileFlags, 0, &vertexShader, nullptr));
			ThrowIfFailed(D3DCompileFromFile(
				L"PixelShader.hlsl",
				nullptr, nullptr, "main", "ps_5_0", compileFlags, 0, &pixelShader, nullptr));

			D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
			{
				{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
				{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
			};

			// Describe and create the graphcis pipeline state object (PSO).
			D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
			psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
			psoDesc.pRootSignature = m_rootSignature.Get();
			psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
			psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
			psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
			psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			psoDesc.DepthStencilState.DepthEnable = FALSE;
			psoDesc.DepthStencilState.StencilEnable = FALSE;
			psoDesc.SampleMask = UINT_MAX;
			psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			psoDesc.NumRenderTargets = 1;
			psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
			psoDesc.SampleDesc.Count = 1;
			ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
		}

		// Create the vertex buffer.
		ComPtr<ID3D12Resource> vertexUploadHeap;
		{
			Vertex vertexList[] =
			{
				{ -0.5f,  0.5f, 0.0f, 0.0f, 0.0f },
				{  0.5f, -0.5f, 0.0f, 1.0f, 1.0f },
				{ -0.5f, -0.5f, 0.0f, 0.0f, 1.0f },
				{  0.5f,  0.5f, 0.0f, 1.0f, 0.0f },
			};

			int vertexBufferSize = sizeof(vertexList);

			auto buffer = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);

			auto defaultHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
			ThrowIfFailed(m_device->CreateCommittedResource(
				&defaultHeapProps,
				D3D12_HEAP_FLAG_NONE,
				&buffer,
				D3D12_RESOURCE_STATE_COPY_DEST,
				nullptr,
				IID_PPV_ARGS(&m_vertexBuffer)));

			m_vertexBuffer->SetName(L"Vertex Buffer Resource Heap");

			auto uploadHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
			ThrowIfFailed(m_device->CreateCommittedResource(
				&uploadHeapProps,
				D3D12_HEAP_FLAG_NONE, // no flags
				&buffer,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&vertexUploadHeap)));

			vertexUploadHeap->SetName(L"Vertex Buffer Upload Resource Heap");

			D3D12_SUBRESOURCE_DATA vertexData = {};
			vertexData.pData = reinterpret_cast<BYTE*>(vertexList);
			vertexData.RowPitch = vertexBufferSize;
			vertexData.SlicePitch = vertexBufferSize;

			UpdateSubresources(m_commandList.Get(), m_vertexBuffer.Get(), vertexUploadHeap.Get(), 0, 0, 1, &vertexData);

			auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
				m_vertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
			m_commandList->ResourceBarrier(1, &barrier);

			m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
			m_vertexBufferView.StrideInBytes = sizeof(Vertex);
			m_vertexBufferView.SizeInBytes = vertexBufferSize;
		}

		// Create Index Buffer
		ComPtr<ID3D12Resource> indexUploadHeap;
		{
			DWORD indexList[] = {
				0, 1, 2,
				0, 3, 1,
			};

			int indexBufferSize = sizeof(indexList);

			auto defaultHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
			auto buffer = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);
			ThrowIfFailed(m_device->CreateCommittedResource(
				&defaultHeapProps,
				D3D12_HEAP_FLAG_NONE, // no flags
				&buffer,
				D3D12_RESOURCE_STATE_COPY_DEST,
				nullptr,
				IID_PPV_ARGS(&m_indexBuffer)));

			m_indexBuffer->SetName(L"Index Buffer Resource Heap");

			auto uploadHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
			ThrowIfFailed(m_device->CreateCommittedResource(
				&uploadHeapProps,
				D3D12_HEAP_FLAG_NONE,
				&buffer,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&indexUploadHeap)));

			indexUploadHeap->SetName(L"Index Buffer Upload Resource Heap");

			D3D12_SUBRESOURCE_DATA indexData = {};
			indexData.pData = reinterpret_cast<BYTE*>(indexList);
			indexData.RowPitch = indexBufferSize;
			indexData.SlicePitch = indexBufferSize;

			UpdateSubresources(m_commandList.Get(), m_indexBuffer.Get(), indexUploadHeap.Get(), 0, 0, 1, &indexData);

			auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
				m_indexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);
			m_commandList->ResourceBarrier(1, &barrier);

			m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
			m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
			m_indexBufferView.SizeInBytes = indexBufferSize;
		}

		CD3DX12_CPU_DESCRIPTOR_HANDLE handle(m_basicHeap->GetCPUDescriptorHandleForHeapStart());

		{
			const UINT constantBufferSize = sizeof(SceneConstantBuffer);

			auto uploadHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
			auto buffer = CD3DX12_RESOURCE_DESC::Buffer(constantBufferSize);
			ThrowIfFailed(m_device->CreateCommittedResource(
				&uploadHeapProps,
				D3D12_HEAP_FLAG_NONE,
				&buffer,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&m_constantBuffer)));

			CD3DX12_RANGE readRange(0, 0);
			ThrowIfFailed(m_constantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_pCbvDataBegin)));
			memcpy(m_pCbvDataBegin, &m_constantBufferData, sizeof(m_constantBufferData));

			// Describe and create a constant buffer view.
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
			cbvDesc.BufferLocation = m_constantBuffer->GetGPUVirtualAddress();
			cbvDesc.SizeInBytes = (constantBufferSize + 255) & ~255; // 256-byte 정렬
			m_device->CreateConstantBufferView(&cbvDesc, handle);

			handle.Offset(1, m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
		}
		

		// Create the texture.
		ComPtr<ID3D12Resource> textureUploadHeap;
		auto image = std::make_unique<ScratchImage>();
		{
			ThrowIfFailed(DirectX::LoadFromWICFile(L"wall.jpg", DirectX::WIC_FLAGS_NONE, nullptr, *image));

			DirectX::TexMetadata metaData = image.get()->GetMetadata();
			ThrowIfFailed(CreateTexture(m_device.Get(), metaData, &m_texture));

			std::vector<D3D12_SUBRESOURCE_DATA> subresources;
			ThrowIfFailed(PrepareUpload(m_device.Get(), image.get()->GetImages(), image.get()->GetImageCount(), metaData, subresources));

			// upload is implemented by application developer. Here's one solution using <d3dx12.h>
			const UINT64 uploadBufferSize = GetRequiredIntermediateSize(m_texture.Get(), 0, static_cast<unsigned int>(subresources.size()));

			auto uploadHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
			auto buffer = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
			ThrowIfFailed(m_device->CreateCommittedResource(
				&uploadHeapProps,
				D3D12_HEAP_FLAG_NONE,
				&buffer,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&textureUploadHeap)));

			UpdateSubresources(
				m_commandList.Get(), m_texture.Get(), textureUploadHeap.Get(),
				0, 0, static_cast<unsigned int>(subresources.size()), subresources.data());

			auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
				m_texture.Get(), // 텍스처 리소스
				D3D12_RESOURCE_STATE_COPY_DEST, // 이전 상태
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE // 새로운 상태
			);
			m_commandList->ResourceBarrier(1, &barrier);

			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Format = metaData.format; // 텍스처의 포맷
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MipLevels = static_cast<UINT>(metaData.mipLevels);

			m_device->CreateShaderResourceView(
				m_texture.Get(), // 텍스처 리소스
				&srvDesc, // SRV 설명
				m_basicHeap->GetCPUDescriptorHandleForHeapStart() // 디스크립터 힙의 핸들
			);

		}

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_sceneRTVHeap->GetCPUDescriptorHandleForHeapStart());
		CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(m_sceneSRVHeap->GetCPUDescriptorHandleForHeapStart());
		{
			for (int n = 0; n < FrameCount; n++)
			{
				D3D12_RESOURCE_DESC renderTargetDesc = {};
				renderTargetDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; // 2차원 텍스처
				renderTargetDesc.Width = m_sceneSize.x;  // 텍스처의 너비
				renderTargetDesc.Height = m_sceneSize.y; // 텍스처의 높이
				renderTargetDesc.DepthOrArraySize = 1; // 단일 텍스처
				renderTargetDesc.MipLevels = 1; // MipMap 수준
				renderTargetDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // 포맷
				renderTargetDesc.SampleDesc.Count = 1; // 멀티샘플링 비활성화
				renderTargetDesc.SampleDesc.Quality = 0;
				renderTargetDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
				renderTargetDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET; // 렌더 타겟 플래그

				D3D12_CLEAR_VALUE clearValue = {};
				clearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				clearValue.Color[0] = 0.0f;
				clearValue.Color[1] = 0.0f;
				clearValue.Color[2] = 0.0f;
				clearValue.Color[3] = 1.0f;

				auto defaultHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
				ThrowIfFailed(m_device->CreateCommittedResource(
					&defaultHeapProps,
					D3D12_HEAP_FLAG_NONE,
					&renderTargetDesc,
					D3D12_RESOURCE_STATE_RENDER_TARGET,
					&clearValue,
					IID_PPV_ARGS(&m_sceneRenderTargets[n])
				));

				// RTV 생성
				m_device->CreateRenderTargetView(m_sceneRenderTargets[n].Get(), nullptr, rtvHandle);
				rtvHandle.Offset(1, m_rtvDescriptorSize);

				// SRV 생성
				D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
				srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
				srvDesc.Texture2D.MipLevels = 1;
				srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

				m_device->CreateShaderResourceView(m_sceneRenderTargets[n].Get(), &srvDesc, srvHandle);
				srvHandle.Offset(1, m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
			}
		}

		ThrowIfFailed(m_commandList->Close());

		ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
		m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		m_fenceValue[m_frameIndex] = 1;
		WaitForPreviousFrame();
	}

	void EngineBase::LoadGUI()
	{
		// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

		// Setup Dear ImGui style
		ImGui::StyleColorsDark();
		//ImGui::StyleColorsLight();

		// Setup Platform/Renderer backends
		ImGui_ImplWin32_Init(WindowApplication::WinApp::m_hwnd);

		ImGui_ImplDX12_InitInfo init_info = {};
		init_info.Device = m_device.Get();
		init_info.CommandQueue = m_commandQueue.Get();
		init_info.NumFramesInFlight = FrameCount;
		init_info.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		init_info.DSVFormat = DXGI_FORMAT_UNKNOWN;
		// Allocating SRV descriptors (for textures) is up to the application, so we provide callbacks.
		// (current version of the backend will only allocate one descriptor, future versions will need to allocate more)
		init_info.SrvDescriptorHeap = m_imguiHeap.Get();
		init_info.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_handle) { return m_srvAlloc.Alloc(out_cpu_handle, out_gpu_handle); };
		init_info.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle) { return m_srvAlloc.Free(cpu_handle, gpu_handle); };
		ImGui_ImplDX12_Init(&init_info);
	}

	void EngineBase::Update()
	{

		//memcpy(m_pCbvDataBegin, &m_constantBufferData, sizeof(m_constantBufferData));
	}

	void EngineBase::Render()
	{
		// Record all the commands we need to render the scene into the command list.
		PopulateCommandList();

		// Execute the command list.
		ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
		m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		// Present the frame.
		ThrowIfFailed(m_swapChain->Present(0, 0));

		WaitForPreviousFrame();
	}

	void EngineBase::Destroy()
	{

		WaitForPreviousFrame();

		CloseHandle(m_fenceEvent);

		// Cleanup
		ImGui_ImplDX12_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();

		// COM 해제
		CoUninitialize();
	}

	void EngineBase::UpdateGUI()
	{
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();

		ImGui::NewFrame();

		ImGui::SetNextWindowPos(ImVec2(5, 5)); // (x, y)는 화면의 절대 좌표
		ImGui::Begin("Scene Control");
		ImGui::Text("Average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

		ImGui::End();

		UpdateSceneViewer();

		// Rendering
		ImGui::Render();

	}

	void EngineBase::UpdateSceneViewer()
	{
		ImGui::SetNextWindowPos(ImVec2(300, 5)); // (x, y)는 화면의 절대 좌표
		ImGui::Begin("Scene 1");

		ImVec2 currentSize = ImGui::GetWindowSize();
		// 크기 변경 여부 확인
		if (currentSize.x != m_sceneSize.x || currentSize.y != m_sceneSize.y) {
			m_sceneSize = currentSize; // 업데이트
			m_aspectRatio = static_cast<float>(m_sceneSize.x) / static_cast<float>(m_sceneSize.y);

			WaitForPreviousFrame();

			for (UINT n = 0; n < FrameCount; n++)
			{
				m_sceneRenderTargets[n].Reset();
				m_fenceValue[n] = m_fenceValue[m_frameIndex];
			}

			// Reset the frame index to the current back buffer index.
			m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

			m_viewport.Width = m_sceneSize.x;
			m_viewport.Height = m_sceneSize.y;

			// Create frame resources.
			CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_sceneRTVHeap->GetCPUDescriptorHandleForHeapStart());
			CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(m_sceneSRVHeap->GetCPUDescriptorHandleForHeapStart());
			for (int n = 0; n < FrameCount; n++)
			{
				D3D12_RESOURCE_DESC renderTargetDesc = {};
				renderTargetDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; // 2차원 텍스처
				renderTargetDesc.Width = m_sceneSize.x;  // 텍스처의 너비
				renderTargetDesc.Height = m_sceneSize.y; // 텍스처의 높이
				renderTargetDesc.DepthOrArraySize = 1; // 단일 텍스처
				renderTargetDesc.MipLevels = 1; // MipMap 수준
				renderTargetDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // 포맷
				renderTargetDesc.SampleDesc.Count = 1; // 멀티샘플링 비활성화
				renderTargetDesc.SampleDesc.Quality = 0;
				renderTargetDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
				renderTargetDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET; // 렌더 타겟 플래그

				D3D12_CLEAR_VALUE clearValue = {};
				clearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				clearValue.Color[0] = 0.0f;
				clearValue.Color[1] = 0.0f;
				clearValue.Color[2] = 0.0f;
				clearValue.Color[3] = 1.0f;

				auto defaultHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
				ThrowIfFailed(m_device->CreateCommittedResource(
					&defaultHeapProps,
					D3D12_HEAP_FLAG_NONE,
					&renderTargetDesc,
					D3D12_RESOURCE_STATE_RENDER_TARGET,
					&clearValue,
					IID_PPV_ARGS(&m_sceneRenderTargets[n])
				));

				// RTV 생성
				m_device->CreateRenderTargetView(m_sceneRenderTargets[n].Get(), nullptr, rtvHandle);
				rtvHandle.Offset(1, m_rtvDescriptorSize);

				// SRV 생성
				D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
				srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
				srvDesc.Texture2D.MipLevels = 1;
				srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

				m_device->CreateShaderResourceView(m_sceneRenderTargets[n].Get(), &srvDesc, srvHandle);
				srvHandle.Offset(1, m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
			}
		}

		UINT srvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		D3D12_GPU_DESCRIPTOR_HANDLE srvHandle = m_sceneSRVHeap->GetGPUDescriptorHandleForHeapStart();
		srvHandle.ptr += m_frameIndex * srvDescriptorSize;
		ImVec2 contentSize = ImGui::GetContentRegionAvail(); // 창 내부 가용 공간 확인
		ImGui::Image(srvHandle.ptr, contentSize);

		ImGui::End();
	}

	void EngineBase::PopulateCommandList()
	{
		// Reset CommandList
		ThrowIfFailed(m_commandAllocator[m_frameIndex]->Reset());
		ThrowIfFailed(m_commandList->Reset(m_commandAllocator[m_frameIndex].Get(), m_pipelineState.Get()));

		auto presentToRT = CD3DX12_RESOURCE_BARRIER::Transition(
			m_renderTargets[m_frameIndex].Get(),
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		m_commandList->ResourceBarrier(1, &presentToRT);

		RenderScene();

		auto RTToPresent = CD3DX12_RESOURCE_BARRIER::Transition(
			m_renderTargets[m_frameIndex].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		m_commandList->ResourceBarrier(1, &RTToPresent);

		ThrowIfFailed(m_commandList->Close());
	}

	void EngineBase::RenderScene()
	{

		m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());

		// Set DescriptorHeap
		ID3D12DescriptorHeap* ppHeaps[] = { m_basicHeap.Get() };
		m_commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

		m_commandList->SetGraphicsRootDescriptorTable(0, m_basicHeap->GetGPUDescriptorHandleForHeapStart());

		CD3DX12_CPU_DESCRIPTOR_HANDLE viewRTVHandle(m_sceneRTVHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
		const float whiteColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		m_commandList->ClearRenderTargetView(viewRTVHandle, whiteColor, 0, nullptr);
		m_commandList->OMSetRenderTargets(1, &viewRTVHandle, FALSE, nullptr);
		m_commandList->RSSetViewports(1, &m_viewport);
		m_commandList->RSSetScissorRects(1, &m_scissorRect);
		m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
		m_commandList->IASetIndexBuffer(&m_indexBufferView);
		m_commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);

		CD3DX12_RESOURCE_BARRIER viewRTToSR = CD3DX12_RESOURCE_BARRIER::Transition(
			m_sceneRenderTargets[m_frameIndex].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,        // 이전 상태
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE // 이후 상태
		);
		m_commandList->ResourceBarrier(1, &viewRTToSR);

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);

		const float blackColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		m_commandList->ClearRenderTargetView(rtvHandle, blackColor, 0, nullptr);
		m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_commandList.Get());

		CD3DX12_RESOURCE_BARRIER viewSRToRT = CD3DX12_RESOURCE_BARRIER::Transition(
			m_sceneRenderTargets[m_frameIndex].Get(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_RENDER_TARGET
		);
		m_commandList->ResourceBarrier(1, &viewSRToRT);
	}

	void EngineBase::WaitForPreviousFrame()
	{
		// WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
		// This is code implemented as such for simplicity. The D3D12HelloFrameBuffering
		// sample illustrates how to use fences for efficient resource usage and to
		// maximize GPU utilization.

		// Signal and increment the fence value.
		const UINT64 fence = m_fenceValue[m_frameIndex];
		ThrowIfFailed(m_commandQueue->Signal(m_fence[m_frameIndex].Get(), fence));
		m_fenceValue[m_frameIndex]++;

		// Wait until the previous frame is finished.
		if (m_fence[m_frameIndex]->GetCompletedValue() < fence)
		{
			ThrowIfFailed(m_fence[m_frameIndex]->SetEventOnCompletion(fence, m_fenceEvent));
			WaitForSingleObject(m_fenceEvent, INFINITE);
		}

		m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
	}

	// Helper function for acquiring the first available hardware adapter that supports Direct3D 12.
	// If no such adapter can be found, *ppAdapter will be set to nullptr.
	_Use_decl_annotations_
		void EngineBase::GetHardwareAdapter(
			IDXGIFactory1* pFactory,
			IDXGIAdapter1** ppAdapter,
			bool requestHighPerformanceAdapter)
	{
		*ppAdapter = nullptr;

		ComPtr<IDXGIAdapter1> adapter;

		ComPtr<IDXGIFactory6> factory6;
		if (SUCCEEDED(pFactory->QueryInterface(IID_PPV_ARGS(&factory6))))
		{
			for (
				UINT adapterIndex = 0;
				SUCCEEDED(factory6->EnumAdapterByGpuPreference(
					adapterIndex,
					requestHighPerformanceAdapter == true ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_UNSPECIFIED,
					IID_PPV_ARGS(&adapter)));
					++adapterIndex)
			{
				DXGI_ADAPTER_DESC1 desc;
				adapter->GetDesc1(&desc);

				if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
				{
					// Don't select the Basic Render Driver adapter.
					// If you want a software adapter, pass in "/warp" on the command line.
					continue;
				}

				// Check to see whether the adapter supports Direct3D 12, but don't create the
				// actual device yet.
				if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
				{
					break;
				}
			}
		}

		if (adapter.Get() == nullptr)
		{
			for (UINT adapterIndex = 0; SUCCEEDED(pFactory->EnumAdapters1(adapterIndex, &adapter)); ++adapterIndex)
			{
				DXGI_ADAPTER_DESC1 desc;
				adapter->GetDesc1(&desc);

				if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
				{
					// Don't select the Basic Render Driver adapter.
					// If you want a software adapter, pass in "/warp" on the command line.
					continue;
				}

				// Check to see whether the adapter supports Direct3D 12, but don't create the
				// actual device yet.
				if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
				{
					break;
				}
			}
		}

		*ppAdapter = adapter.Detach();
	}
}