#include "FrameResource.h"

FrameResource::FrameResource(
	ComPtr<ID3D12Device>& device, float width, float height, UINT frameIndex)
{
	m_width = width;
	m_height = height;
	m_frameIndex = frameIndex;

	ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)));
	ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_commandList)));
	ThrowIfFailed(m_commandList->Close());

	CreateConstUploadBuffer(device, m_commandList, m_globalConstsUploadHeap, m_globalConstsBufferData, m_globalConstsBufferDataBegin);

	CreateConstUploadBuffer(device, m_commandList, m_reflectGlobalConstsUploadHeap, m_reflectGlobalConstsBufferData, m_reflectGlobalConstsBufferDataBegin);

	CreateConstUploadBuffer(device, m_commandList, m_cubemapIndexConstsUploadHeap, m_cubemapIndexConstsBufferData, m_cubemapIndexConstsBufferDataBegin);

	InitializeDescriptorHeaps(device);
}

FrameResource::~FrameResource()
{

}

void FrameResource::Update(
	shared_ptr<Camera>& camera, 
	Light& m_lightFromGUI,
	XMFLOAT4& mirrorPlane,
	GlobalConstants& globalConsts,
	CubemapIndexConstants& cubemapIndexConsts)
{
	XMMATRIX view = camera->GetViewMatrix();
	XMMATRIX viewTrans = XMMatrixTranspose(view);
	XMStoreFloat4x4(&m_globalConstsBufferData.view, viewTrans);

	XMMATRIX proj = camera->GetProjectionMatrix(XMConvertToRadians(45.0f), camera->m_aspectRatio, 0.1f, 1000.0f);
	XMMATRIX projTrans = XMMatrixTranspose(proj);
	XMStoreFloat4x4(&m_globalConstsBufferData.proj, projTrans);

	XMMATRIX invProj = XMMatrixInverse(nullptr, proj);
	XMMATRIX invProjTrans = XMMatrixTranspose(invProj);
	XMStoreFloat4x4(&m_globalConstsBufferData.invProj, invProjTrans);

	m_globalConstsBufferData.eyeWorld = camera->GetEyePos();
	m_globalConstsBufferData.light[1] = m_lightFromGUI;
	m_globalConstsBufferData.strengthIBL = globalConsts.strengthIBL;
	m_globalConstsBufferData.choiceEnvMap = globalConsts.choiceEnvMap;
	m_globalConstsBufferData.envLodBias = globalConsts.envLodBias;

	memcpy(m_globalConstsBufferDataBegin, &m_globalConstsBufferData, sizeof(m_globalConstsBufferData));

	// Reflect
	m_reflectGlobalConstsBufferData = m_globalConstsBufferData;

	XMVECTOR plane = XMLoadFloat4(&mirrorPlane);
	XMMATRIX reflectionMatrix = XMMatrixReflect(plane);
	XMMATRIX reflectedViewMatrix = XMMatrixMultiply(reflectionMatrix, view);
	XMMATRIX reflectedViewMatrixTrans = XMMatrixTranspose(reflectedViewMatrix);
	XMStoreFloat4x4(&m_reflectGlobalConstsBufferData.view, reflectedViewMatrixTrans);

	memcpy(m_reflectGlobalConstsBufferDataBegin, &m_reflectGlobalConstsBufferData, sizeof(m_reflectGlobalConstsBufferData));

	m_cubemapIndexConstsBufferData = cubemapIndexConsts;

	memcpy(m_cubemapIndexConstsBufferDataBegin, &m_cubemapIndexConstsBufferData, sizeof(m_cubemapIndexConstsBufferData));
}

void FrameResource::InitializeDescriptorHeaps(
	ComPtr<ID3D12Device>& device)
{
	// Resolved
	{
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = 1;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_resolvedRTVHeap)));

		D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
		srvHeapDesc.NumDescriptors = 1;
		srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		ThrowIfFailed(device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_resolvedSRVHeap)));

		UINT sampleCount = 1;
		CreateBuffer(device, m_resolvedBuffers, m_width, m_height, 0, sampleCount, DXGI_FORMAT_R16G16B16A16_FLOAT, D3D12_SRV_DIMENSION_TEXTURE2D, D3D12_RESOURCE_STATE_RESOLVE_DEST, m_resolvedRTVHeap, m_resolvedSRVHeap);

		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
		dsvHeapDesc.NumDescriptors = 1;
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_resolvedDSVHeap)));

		D3D12_RESOURCE_DESC depthStencilDesc = {};
		depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		depthStencilDesc.Width = static_cast<UINT>(m_width);; // 화면 너비
		depthStencilDesc.Height = static_cast<UINT>(m_height); // 화면 높이
		depthStencilDesc.DepthOrArraySize = 1;
		depthStencilDesc.MipLevels = 1;
		depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depthStencilDesc.SampleDesc.Count = sampleCount;
		depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		D3D12_CLEAR_VALUE clearValue = {};
		clearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		clearValue.DepthStencil.Depth = 1.0f;
		clearValue.DepthStencil.Stencil = 0;

		auto defaultHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		ThrowIfFailed(device->CreateCommittedResource(
			&defaultHeapProps,
			D3D12_HEAP_FLAG_NONE,
			&depthStencilDesc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&clearValue,
			IID_PPV_ARGS(&m_floatDSBuffer)
		));

		// DSV 핸들 생성
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

		device->CreateDepthStencilView(m_resolvedDSBuffer.Get(), &dsvDesc, m_resolvedDSVHeap->GetCPUDescriptorHandleForHeapStart());
	}

	// MSAA
	{
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = 1;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_floatRTVHeap)));

		D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
		srvHeapDesc.NumDescriptors = 1;
		srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		ThrowIfFailed(device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_floatSRVHeap)));

		UINT sampleCount = 4;
		CreateBuffer(device, m_floatBuffers, static_cast<UINT>(m_width), static_cast<UINT>(m_height), 0, sampleCount, DXGI_FORMAT_R16G16B16A16_FLOAT, D3D12_SRV_DIMENSION_TEXTURE2DMS,
			D3D12_RESOURCE_STATE_RENDER_TARGET, m_floatRTVHeap, m_floatSRVHeap);

		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
		dsvHeapDesc.NumDescriptors = 1;
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_floatDSVHeap)));

		D3D12_RESOURCE_DESC depthStencilDesc = {};
		depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		depthStencilDesc.Width = static_cast<UINT>(m_width);; // 화면 너비
		depthStencilDesc.Height = static_cast<UINT>(m_height); // 화면 높이
		depthStencilDesc.DepthOrArraySize = 1;
		depthStencilDesc.MipLevels = 1;
		depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depthStencilDesc.SampleDesc.Count = sampleCount;
		depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		D3D12_CLEAR_VALUE clearValue = {};
		clearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		clearValue.DepthStencil.Depth = 1.0f;
		clearValue.DepthStencil.Stencil = 0;

		auto defaultHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		ThrowIfFailed(device->CreateCommittedResource(
			&defaultHeapProps,
			D3D12_HEAP_FLAG_NONE,
			&depthStencilDesc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&clearValue,
			IID_PPV_ARGS(&m_floatDSBuffer)
		));

		// DSV 핸들 생성
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

		device->CreateDepthStencilView(m_floatDSBuffer.Get(), &dsvDesc, m_floatDSVHeap->GetCPUDescriptorHandleForHeapStart());
	}

	// Fog
	{
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = 1;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_fogRTVHeap)));

		D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
		srvHeapDesc.NumDescriptors = 1;
		srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		ThrowIfFailed(device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_fogSRVHeap)));

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_fogRTVHeap->GetCPUDescriptorHandleForHeapStart());
		CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(m_fogSRVHeap->GetCPUDescriptorHandleForHeapStart());

		CreateBuffer(device, m_fogBuffer, static_cast<UINT>(m_width), static_cast<UINT>(m_height), 0, 1, DXGI_FORMAT_R16G16B16A16_FLOAT, D3D12_SRV_DIMENSION_TEXTURE2D,
			D3D12_RESOURCE_STATE_RENDER_TARGET, m_fogRTVHeap, m_fogSRVHeap);
	}

	// PostEffects DepthOnly
	{
		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
		dsvHeapDesc.NumDescriptors = 1; // 필요 시 증가
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_depthOnlyDSVHeap)));

		D3D12_RESOURCE_DESC depthStencilDesc = {};
		depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		depthStencilDesc.Width = static_cast<UINT>(m_width);
		depthStencilDesc.Height = static_cast<UINT>(m_height);
		depthStencilDesc.DepthOrArraySize = 1;
		depthStencilDesc.MipLevels = 1;
		depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
		depthStencilDesc.SampleDesc.Count = 1; // MSAA 끔
		depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		D3D12_CLEAR_VALUE clearValue = {};
		clearValue.Format = DXGI_FORMAT_D32_FLOAT;
		clearValue.DepthStencil.Depth = 1.0f;
		clearValue.DepthStencil.Stencil = 0;

		auto defaultHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		ThrowIfFailed(device->CreateCommittedResource(
			&defaultHeapProps,
			D3D12_HEAP_FLAG_NONE,
			&depthStencilDesc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&clearValue,
			IID_PPV_ARGS(&m_depthOnlyDSBuffer)
		));

		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

		device->CreateDepthStencilView(m_depthOnlyDSBuffer.Get(), &dsvDesc, m_depthOnlyDSVHeap->GetCPUDescriptorHandleForHeapStart());

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;

		// TextureManager에서 관리
		//m_textureManager->CreateSRV(device, m_depthOnlyDSBuffer, srvDesc, m_globalConstsBufferData);
	}
}