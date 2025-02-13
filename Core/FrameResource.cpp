#include "FrameResource.h"

FrameResource::FrameResource(
	ComPtr<ID3D12Device>& device, float width, float height, UINT frameIndex)
{
	m_width = width;
	m_height = height;

	m_shadowWidth = 1024;
	m_shadowHeight = 1024;

	m_shadowViewport.TopLeftX = 0.0f;
	m_shadowViewport.TopLeftY = 0.0f;
	m_shadowViewport.Width = m_shadowWidth;
	m_shadowViewport.Height = m_shadowHeight;
	m_shadowViewport.MinDepth = 0.0f;
	m_shadowViewport.MaxDepth = 1.0f;

	m_shadowScissorRect.left = 0;
	m_shadowScissorRect.top = 0;
	m_shadowScissorRect.right = static_cast<LONG>(m_shadowWidth);
	m_shadowScissorRect.bottom = static_cast<LONG>(m_shadowHeight);

	m_frameIndex = frameIndex;
	
	rtvSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	cbvSrvSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	dsvSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	for (UINT i = 0; i < commandListCount; i++)
	{
		ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator[i])));
		ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator[i].Get(), nullptr, IID_PPV_ARGS(&m_commandList[i])));
		ThrowIfFailed(m_commandList[i]->Close());
	}

	m_batchSubmit[0] = m_commandList[0].Get();

	CreateConstUploadBuffer(device, m_globalConstsUploadHeap, m_globalConstsData, m_globalConstsDataBegin);

	for (UINT i = 0; i < MAX_LIGHTS; i++)
		CreateConstUploadBuffer(device, m_shadowGlobalConstsUploadHeap[i], m_shadowGlobalConstsData[i], m_shadowGlobalConstsDataBegin[i]);

	CreateConstUploadBuffer(device, m_reflectConstsUploadHeap, m_reflectConstsData, m_reflectConstsDataBegin);

	CreateConstUploadBuffer(device, m_cubemapIndexConstsUploadHeap, m_cubemapIndexConstsData, m_cubemapIndexConstsDataBegin);
}

FrameResource::~FrameResource()
{

}

void FrameResource::Update(
	shared_ptr<Camera>& camera,
	XMFLOAT4& mirrorPlane,
	GlobalConstants& globalConsts,
	GlobalConstants(&shadowGlobalConsts)[MAX_LIGHTS],
	CubemapIndexConstants& cubemapIndexConsts)
{
	XMMATRIX view = camera->GetViewMatrix();
	XMMATRIX viewTrans = XMMatrixTranspose(view);
	XMStoreFloat4x4(&m_globalConstsData.view, viewTrans);

	XMMATRIX proj = camera->GetProjectionMatrix();
	XMStoreFloat4x4(&m_globalConstsData.proj, XMMatrixTranspose(proj));

	XMMATRIX viewProjTrans = XMMatrixMultiplyTranspose(view, proj);
	XMStoreFloat4x4(&m_globalConstsData.viewProj, viewProjTrans);

	XMMATRIX invProj = XMMatrixInverse(nullptr, proj);
	XMStoreFloat4x4(&m_globalConstsData.invProj, XMMatrixTranspose(invProj));

	m_globalConstsData.eyeWorld = camera->GetEyePos();
	m_globalConstsData.strengthIBL = globalConsts.strengthIBL;
	m_globalConstsData.choiceEnvMap = globalConsts.choiceEnvMap;
	m_globalConstsData.envLodBias = globalConsts.envLodBias;
	m_globalConstsData.light[0] = globalConsts.light[0];
	m_globalConstsData.light[1] = globalConsts.light[1];

	memcpy(m_globalConstsDataBegin, &m_globalConstsData, sizeof(m_globalConstsData));

	// Shadow
	for (UINT i = 0; i < MAX_LIGHTS; i++)
	{
		m_shadowGlobalConstsData[i] = shadowGlobalConsts[i];
		memcpy(m_shadowGlobalConstsDataBegin[i], &m_shadowGlobalConstsData[i], sizeof(m_shadowGlobalConstsData[i]));
	}

	// Reflect
	m_reflectConstsData = m_globalConstsData;

	XMVECTOR plane = XMLoadFloat4(&mirrorPlane);
	XMMATRIX reflectionMatrix = XMMatrixReflect(plane);
	XMMATRIX reflectedViewMatrix = XMMatrixMultiply(reflectionMatrix, view);
	XMStoreFloat4x4(&m_reflectConstsData.view, XMMatrixTranspose(reflectedViewMatrix));

	memcpy(m_reflectConstsDataBegin, &m_reflectConstsData, sizeof(m_reflectConstsData));

	m_cubemapIndexConstsData = cubemapIndexConsts;

	memcpy(m_cubemapIndexConstsDataBegin, &m_cubemapIndexConstsData, sizeof(m_cubemapIndexConstsData));
}

void FrameResource::InitializeDescriptorHeaps(
	ComPtr<ID3D12Device>& device,
	shared_ptr<TextureManager>& textureManager)
{
	// Resolved
	{
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = 1;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_resolvedRTVHeap)));

		UINT sampleCount = 1;
		CreateBuffer(device, m_resolvedBuffers, m_width, m_height, sampleCount,
			DXGI_FORMAT_R16G16B16A16_FLOAT, D3D12_SRV_DIMENSION_TEXTURE2D, D3D12_RESOURCE_STATE_RESOLVE_DEST,
			m_resolvedRTVHeap, 0, textureManager->m_textureHeap, textureManager->m_textureCnt);
		m_globalConstsData.resolvedSRVIndex = textureManager->m_textureCnt++;
	}

	// MSAA
	{
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = 1;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_floatRTVHeap)));

		UINT sampleCount = 4;
		CreateBuffer(device, m_floatBuffers, static_cast<UINT>(m_width), static_cast<UINT>(m_height), sampleCount,
			DXGI_FORMAT_R16G16B16A16_FLOAT, D3D12_SRV_DIMENSION_TEXTURE2DMS, D3D12_RESOURCE_STATE_RENDER_TARGET,
			m_floatRTVHeap, 0, nullptr, 0);

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

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_fogRTVHeap->GetCPUDescriptorHandleForHeapStart());

		UINT sampleCount = 1;
		CreateBuffer(device, m_fogBuffer, static_cast<UINT>(m_width), static_cast<UINT>(m_height), sampleCount,
			DXGI_FORMAT_R16G16B16A16_FLOAT, D3D12_SRV_DIMENSION_TEXTURE2D, D3D12_RESOURCE_STATE_RENDER_TARGET,
			m_fogRTVHeap, 0, textureManager->m_textureHeap, textureManager->m_textureCnt);
		m_globalConstsData.fogSRVIndex = textureManager->m_textureCnt++;
	}

	// DepthOnly
	{
		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
		dsvHeapDesc.NumDescriptors = 1;
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_depthOnlyDSVHeap)));

		D3D12_RESOURCE_DESC depthStencilDesc = {};
		depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		depthStencilDesc.Width = static_cast<UINT>(m_width);
		depthStencilDesc.Height = static_cast<UINT>(m_height);
		depthStencilDesc.DepthOrArraySize = 1;
		depthStencilDesc.MipLevels = 1;
		depthStencilDesc.Format = DXGI_FORMAT_R32_TYPELESS;
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

		CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(textureManager->m_heapStartCpu, textureManager->m_textureCnt * cbvSrvSize);
		m_globalConstsData.depthOnlySRVIndex = textureManager->m_textureCnt++;

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;

		device->CreateShaderResourceView(
			m_depthOnlyDSBuffer.Get(),
			&srvDesc,
			srvHandle
		);
	}

	// ShadowDepthOnly
	{
		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
		dsvHeapDesc.NumDescriptors = MAX_LIGHTS;
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_shadowDepthOnlyDSVHeap)));

		D3D12_RESOURCE_DESC depthStencilDesc = {};
		depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		depthStencilDesc.Width = static_cast<UINT>(m_shadowWidth);
		depthStencilDesc.Height = static_cast<UINT>(m_shadowHeight);
		depthStencilDesc.DepthOrArraySize = 1;
		depthStencilDesc.MipLevels = 1;
		depthStencilDesc.Format = DXGI_FORMAT_R32_TYPELESS;
		depthStencilDesc.SampleDesc.Count = 1; // MSAA 끔
		depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		D3D12_CLEAR_VALUE clearValue = {};
		clearValue.Format = DXGI_FORMAT_D32_FLOAT;
		clearValue.DepthStencil.Depth = 1.0f;
		clearValue.DepthStencil.Stencil = 0;

		for (UINT i = 0; i < MAX_LIGHTS; i++)
		{
			auto defaultHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
			ThrowIfFailed(device->CreateCommittedResource(
				&defaultHeapProps,
				D3D12_HEAP_FLAG_NONE,
				&depthStencilDesc,
				D3D12_RESOURCE_STATE_DEPTH_WRITE,
				&clearValue,
				IID_PPV_ARGS(&m_shadowDepthOnlyDSBuffer[i])
			));

			D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
			dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
			dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

			CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_shadowDepthOnlyDSVHeap->GetCPUDescriptorHandleForHeapStart(), i * dsvSize);
			device->CreateDepthStencilView(m_shadowDepthOnlyDSBuffer[i].Get(), &dsvDesc, dsvHandle);

			CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(textureManager->m_heapStartCpu, textureManager->m_textureCnt * cbvSrvSize);
			if (i == 0)
				m_globalConstsData.shadowDepthOnlyStartIndex = textureManager->m_textureCnt;
			textureManager->m_textureCnt++;

			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MipLevels = 1;

			device->CreateShaderResourceView(
				m_shadowDepthOnlyDSBuffer[i].Get(),
				&srvDesc,
				srvHandle
			);
		}
	}

	// Scene
	{
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = 1;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_sceneRTVHeap)));

		UINT sampleCount = 1;
		CreateBuffer(device, m_sceneBuffer, static_cast<UINT>(m_width), static_cast<UINT>(m_height), sampleCount,
			DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_SRV_DIMENSION_TEXTURE2D, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			m_sceneRTVHeap, 0, textureManager->m_textureHeap, textureManager->m_textureCnt);
		m_sceneBufferIndex = textureManager->m_textureCnt++;
	}
}