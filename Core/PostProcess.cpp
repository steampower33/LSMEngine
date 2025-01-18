#include "PostProcess.h"

PostProcess::PostProcess(
	ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& commandList,
	float width, float height)
{
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
	m_bloomLevels = 3;
	m_bufferSize = 1 + 2 * m_bloomLevels;

	CreateDescriptors(device);

	MeshData square = GeometryGenerator::MakeSquare(1.0f);

	m_mesh = make_shared<Mesh>();
	m_mesh->vertexBufferCount = static_cast<UINT>(square.vertices.size());
	m_mesh->indexBufferCount = static_cast<UINT>(square.indices.size());
	CreateVertexBuffer(device, commandList, square.vertices, m_mesh);
	CreateIndexBuffer(device, commandList, square.indices, m_mesh);

	m_pingPong.resize(m_bufferSize);

	// CopyFilter
	shared_ptr<ImageFilter> copyFilter = make_shared<ImageFilter>(device, commandList, width, height, 0);
	m_filters.push_back(copyFilter);
	CreateTex2D(device, m_pingPong[0], static_cast<UINT>(width), static_cast<UINT>(height), 0, m_pingPongRtvHeap, m_pingPongSrvHeap);

	// DownFilter
	for (UINT i = 1; i < m_bloomLevels + 1; i++)
	{
		float div = float(pow(2, i));

		UINT newWidth = static_cast<UINT>(width / div);
		UINT newHeight = static_cast<UINT>(height / div);

		shared_ptr<ImageFilter> downFilter = make_shared<ImageFilter>(device, commandList, newWidth, newHeight, i - 1);
		m_filters.push_back(downFilter);

		CreateTex2D(device, m_pingPong[i], newWidth, newHeight, i, m_pingPongRtvHeap, m_pingPongSrvHeap);
	}

	//// UpFilter
	//for (UINT i = m_bloomLevels - 1; i > 0; i--)
	//{
	//	float div = float(pow(2, i));

	//	UINT newWidth = static_cast<UINT>(width / div);
	//	UINT newHeight = static_cast<UINT>(height / div);

	//	shared_ptr<ImageFilter> upFilter = make_shared<ImageFilter>(device, commandList, newWidth, newHeight, i + 1);
	//	m_filters.push_back(upFilter);
	//}

	// BlurFilter
	for (UINT i = 1; i < m_bloomLevels + 1; i++)
	{
		float div = float(pow(2, i));

		UINT newWidth = static_cast<UINT>(width / div);
		UINT newHeight = static_cast<UINT>(height / div);

		shared_ptr<ImageFilter> blurXFilter = make_shared<ImageFilter>(device, commandList, newWidth, newHeight, i);
		m_blurXFilters.push_back(blurXFilter);

		shared_ptr<ImageFilter> blurYFilter = make_shared<ImageFilter>(device, commandList, newWidth, newHeight, m_bloomLevels + i);
		m_blurYFilters.push_back(blurYFilter);

		CreateTex2D(device, m_pingPong[m_bloomLevels + i], newWidth, newHeight, m_bloomLevels + i, m_pingPongRtvHeap, m_pingPongSrvHeap);
	}

	// BlurCombineFilter
	for (UINT i = m_bloomLevels - 1; i > 0; i--)
	{
		float div = float(pow(2, i));

		UINT newWidth = static_cast<UINT>(width / div);
		UINT newHeight = static_cast<UINT>(height / div);

		shared_ptr<ImageFilter> blurCombineFilter = make_shared<ImageFilter>(device, commandList, newWidth, newHeight, 0);
		if (i == m_bloomLevels - 1)
			blurCombineFilter->UpdateBlurCombineIndex(i, i + 1);
		else
		{
			blurCombineFilter->UpdateBlurCombineIndex(i, m_bloomLevels + i + 1);
		}
		m_blurCombineFilters.push_back(blurCombineFilter);
	}

	// CombineFilter
	m_combineFilter = make_shared<ImageFilter>(device, commandList, width, height, 0);
	m_combineFilter->UpdateBlurCombineIndex(0, 1 + m_bloomLevels);
}

void PostProcess::Update(float threshold, float strength)
{
	m_filters[1]->Update(threshold, 0.0f);

	for (auto& f : m_blurCombineFilters)
	{
		f->Update(0.0f, strength);
	}
	m_combineFilter->Update(0.0f, strength);
}

void PostProcess::UpdateIndex(UINT frameIndex)
{
	m_filters[0]->UpdateIndex(frameIndex);
}

void PostProcess::Render(
	ComPtr<ID3D12Device>& device,
	ComPtr<ID3D12GraphicsCommandList>& commandList,
	ComPtr<ID3D12Resource>& renderTarget,
	ComPtr<ID3D12DescriptorHeap>& rtv,
	ComPtr<ID3D12DescriptorHeap>& srv,
	ComPtr<ID3D12DescriptorHeap>& dsv,
	UINT frameIndex)
{

	commandList->IASetVertexBuffers(0, 1, &m_mesh->vertexBufferView);
	commandList->IASetIndexBuffer(&m_mesh->indexBufferView);

	commandList->SetPipelineState(Graphics::samplingPSO.Get());

	// Copy
	{
		ID3D12DescriptorHeap* heaps[] = { srv.Get() };
		commandList->SetDescriptorHeaps(_countof(heaps), heaps);

		CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle(srv->GetGPUDescriptorHandleForHeapStart());
		commandList->SetGraphicsRootDescriptorTable(6, srvHandle);

		m_filters[0]->Render(commandList, m_pingPongRtvHeap, 0, dsv, srv, m_mesh->indexBufferCount);

		SetBarrier(commandList, m_pingPong[0],
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}

	// DownSampling
	ID3D12DescriptorHeap* heaps[] = { m_pingPongSrvHeap.Get() };
	commandList->SetDescriptorHeaps(_countof(heaps), heaps);
	CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle(m_pingPongSrvHeap->GetGPUDescriptorHandleForHeapStart());
	commandList->SetGraphicsRootDescriptorTable(6, srvHandle);

	for (UINT i = 1; i < m_bloomLevels + 1; i++)
	{
		m_filters[i]->Render(commandList, m_pingPongRtvHeap, rtvSize * i, dsv, m_pingPongSrvHeap, m_mesh->indexBufferCount);

		SetBarrier(commandList, m_pingPong[i],
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}

	// Blur
	for (UINT i = 1; i < m_bloomLevels + 1; i++)
	{
		for (UINT j = 0; j < 5; j++)
		{
			// Blur X
			commandList->SetPipelineState(Graphics::blurXPSO.Get());
			m_blurXFilters[i - 1]->Render(commandList, m_pingPongRtvHeap, rtvSize * (m_bloomLevels + i), dsv, m_pingPongSrvHeap, m_mesh->indexBufferCount);

			SetBarrier(commandList, m_pingPong[m_bloomLevels + i],
				D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

			SetBarrier(commandList, m_pingPong[i],
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

			// Blur Y
			commandList->SetPipelineState(Graphics::blurYPSO.Get());
			m_blurYFilters[i - 1]->Render(commandList, m_pingPongRtvHeap, rtvSize * i, dsv, m_pingPongSrvHeap, m_mesh->indexBufferCount);

			SetBarrier(commandList, m_pingPong[i],
				D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

			SetBarrier(commandList, m_pingPong[m_bloomLevels + i],
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
		}
	}

	commandList->SetPipelineState(Graphics::blurCombinePSO.Get());
	for (UINT i = 0; i < m_bloomLevels - 1; i++)
	{
		m_blurCombineFilters[i]->Render(commandList, m_pingPongRtvHeap, rtvSize * (m_bufferSize - 2 - i), dsv, m_pingPongSrvHeap, m_mesh->indexBufferCount);

		SetBarrier(commandList, m_pingPong[m_bufferSize - 2 - i],
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}

	//// UpSampling
	//for (UINT i = m_bloomLevels - 1; i > 0; i--)
	//{
	//	SetBarrier(commandList, m_pingPong[i],
	//		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

	//	m_filters[m_bloomLevels + m_bloomLevels - i]->Render(commandList, m_pingPongRtvHeap, rtvSize * i, dsv, m_pingPongSrvHeap, m_mesh->indexBufferCount);

	//	SetBarrier(commandList, m_pingPong[i],
	//		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	//}

	SetBarrier(commandList, renderTarget,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

	commandList->SetPipelineState(Graphics::combinePSO.Get());
	m_combineFilter->Render(commandList, rtv, rtvSize * frameIndex, dsv, m_pingPongSrvHeap, m_mesh->indexBufferCount);

	for (UINT i = 0; i < m_bloomLevels * 2; i++)
	{
		SetBarrier(commandList, m_pingPong[i],
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
		DXGI_FORMAT_R32G32B32A32_FLOAT, // 텍스처 포맷
		width,                          // 화면 너비
		height,                         // 화면 높이
		1,                              // arraySize
		1,                              // mipLevels
		1,                              // sampleCount
		0,                              // sampleQuality
		D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET // RT로 사용할 예정이면 플래그 설정
	);

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = DXGI_FORMAT_R32G32B32A32_FLOAT; // 텍스처의 포맷
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

void PostProcess::CreateDescriptors(ComPtr<ID3D12Device>& device)
{
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = m_bufferSize;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_pingPongRtvHeap)));

	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = m_bufferSize;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_pingPongSrvHeap)));
}