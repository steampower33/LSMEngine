#include "PostProcess.h"

PostProcess::PostProcess(
	ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& commandList,
	float width, float height, UINT frameIndex)
{
	m_bloomLevels = Graphics::bloomLevels;
	m_frameIndex = frameIndex;
	Initialize(device, commandList, width, height);
}

PostProcess::~PostProcess()
{

}

void PostProcess::Initialize(
	ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& commandList,
	float width, float height
)
{
	m_bufferSize = 1 + m_bloomLevels;

	CreateDescriptors(device, width, height);

	MeshData square = GeometryGenerator::MakeSquare(1.0f);

	m_mesh = make_shared<Mesh>();
	m_mesh->vertexBufferCount = static_cast<UINT>(square.vertices.size());
	m_mesh->indexBufferCount = static_cast<UINT>(square.indices.size());
	CreateVertexBuffer(device, commandList, square.vertices, m_mesh);
	CreateIndexBuffer(device, commandList, square.indices, m_mesh);

	m_buffer.resize(m_bufferSize);

	// CopyFilter
	m_copyFilter = make_shared<ImageFilter>(device, commandList, width, height, m_frameIndex);
	CreateTex2D(device, m_buffer[0], static_cast<UINT>(width), static_cast<UINT>(height), 0, m_rtvHeap, m_srvHeap);

	// BloomDownFilter
	for (UINT i = 1; i < m_bloomLevels + 1; i++)
	{
		float div = float(pow(2, i));

		UINT newWidth = static_cast<UINT>(width / div);
		UINT newHeight = static_cast<UINT>(height / div);

		shared_ptr<ImageFilter> bloomDownFilter = make_shared<ImageFilter>(device, commandList, newWidth, newHeight, i - 1);
		m_bloomDownFilters.push_back(bloomDownFilter);

		CreateTex2D(device, m_buffer[i], newWidth, newHeight, i, m_rtvHeap, m_srvHeap);
	}

	// BloomUpFilter
	for (UINT i = m_bloomLevels - 1; i > 0; i--)
	{
		float div = float(pow(2, i));

		UINT newWidth = static_cast<UINT>(width / div);
		UINT newHeight = static_cast<UINT>(height / div);

		shared_ptr<ImageFilter> bloomUpFilter = make_shared<ImageFilter>(device, commandList, newWidth, newHeight, i + 1);
		m_bloomUpFilters.push_back(bloomUpFilter);
	}

	// CombineFilter
	m_combineFilter = make_shared<ImageFilter>(device, commandList, width, height, 0);
}

void PostProcess::Update(SamplingConstants& m_combineConsts)
{
	m_copyFilter->Update(m_combineConsts);

	m_combineFilter->Update(m_combineConsts);
}

void PostProcess::Render(
	ComPtr<ID3D12Device>& device,
	ComPtr<ID3D12GraphicsCommandList>& commandList,
	ComPtr<ID3D12Resource>& renderTarget,
	ComPtr<ID3D12DescriptorHeap>& rtv,
	ComPtr<ID3D12DescriptorHeap>& resolvedSRV,
	ComPtr<ID3D12DescriptorHeap>& dsv,
	UINT frameIndex)
{

	commandList->IASetVertexBuffers(0, 1, &m_mesh->vertexBufferView);
	commandList->IASetIndexBuffer(&m_mesh->indexBufferView);

	{
		ID3D12DescriptorHeap* heaps[] = { resolvedSRV.Get() };
		commandList->SetDescriptorHeaps(_countof(heaps), heaps);

		CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle(resolvedSRV->GetGPUDescriptorHandleForHeapStart());
		commandList->SetGraphicsRootDescriptorTable(6, srvHandle);

		commandList->SetPipelineState(Graphics::samplingPSO.Get());
		m_copyFilter->Render(commandList, m_rtvHeap, 0, m_dsvHeap, resolvedSRV, m_mesh->indexBufferCount);

		SetBarrier(commandList, m_buffer[0],
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}

	ID3D12DescriptorHeap* heaps[] = { m_srvHeap.Get() };
	commandList->SetDescriptorHeaps(_countof(heaps), heaps);
	CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle(m_srvHeap->GetGPUDescriptorHandleForHeapStart());
	commandList->SetGraphicsRootDescriptorTable(6, srvHandle);

	// BloomDown
	commandList->SetPipelineState(Graphics::bloomDownPSO.Get());
	for (UINT i = 1; i < m_bloomLevels + 1; i++)
	{
		m_bloomDownFilters[i - 1]->Render(commandList, m_rtvHeap, rtvSize * i, m_dsvHeap, m_srvHeap, m_mesh->indexBufferCount);

		SetBarrier(commandList, m_buffer[i],
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}

	// BloomUp
	commandList->SetPipelineState(Graphics::bloomUpPSO.Get());
	for (UINT i = m_bloomLevels - 1; i > 0; i--)
	{
		SetBarrier(commandList, m_buffer[i],
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

		m_bloomUpFilters[m_bloomLevels - (i + 1)]->Render(commandList, m_rtvHeap, rtvSize * i, m_dsvHeap, m_srvHeap, m_mesh->indexBufferCount);

		SetBarrier(commandList, m_buffer[i],
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}

	commandList->SetPipelineState(Graphics::combinePSO.Get());
	m_combineFilter->Render(commandList, rtv, rtvSize * m_frameIndex, dsv, m_srvHeap, m_mesh->indexBufferCount);

	for (UINT i = 0; i < m_bloomLevels + 1; i++)
	{
		SetBarrier(commandList, m_buffer[i],
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
	}
}

void PostProcess::CreateTex2D(
	ComPtr<ID3D12Device>& device, ComPtr<ID3D12Resource>& texture,
	UINT width, UINT height, UINT index,
	ComPtr<ID3D12DescriptorHeap>& rtvHeap, ComPtr<ID3D12DescriptorHeap>& srvHeap)
{
	auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	auto texDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R16G16B16A16_FLOAT, // 텍스처 포맷
		width,                          // 화면 너비
		height,                         // 화면 높이
		1,                              // arraySize
		1,                              // mipLevels
		1,                              // sampleCount
		0,                              // sampleQuality
		D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET // RT로 사용할 예정이면 플래그 설정
	);

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = DXGI_FORMAT_R16G16B16A16_FLOAT; // 텍스처의 포맷
	clearValue.Color[0] = 0.0f; // Red
	clearValue.Color[1] = 0.2f; // Green
	clearValue.Color[2] = 1.0f; // Blue
	clearValue.Color[3] = 1.0f; // Alpha

	ThrowIfFailed(device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		&clearValue,
		IID_PPV_ARGS(&texture)
	));

	rtvSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	srvSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart(), rtvSize * index);
	CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(srvHeap->GetCPUDescriptorHandleForHeapStart(), srvSize * index);

	// Create a RTV for each frame
	device->CreateRenderTargetView(texture.Get(), nullptr, rtvHandle);

	// Create a SRV for each frame
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = texture->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = texture->GetDesc().MipLevels;

	device->CreateShaderResourceView(
		texture.Get(),
		&srvDesc,
		srvHandle
	);
}

void PostProcess::CreateDescriptors(ComPtr<ID3D12Device>& device, float width, float height)
{
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = m_bufferSize;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));

	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = m_bufferSize;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_srvHeap)));

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap)));

	D3D12_RESOURCE_DESC depthStencilDesc = {};
	depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthStencilDesc.Width = static_cast<UINT>(width);; // 화면 너비
	depthStencilDesc.Height = static_cast<UINT>(height); // 화면 높이
	depthStencilDesc.DepthOrArraySize = 1;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilDesc.SampleDesc.Count = 1;
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
		IID_PPV_ARGS(&m_dsBuffer)
	));

	// DSV 핸들 생성
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

	device->CreateDepthStencilView(m_dsBuffer.Get(), &dsvDesc, m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
}