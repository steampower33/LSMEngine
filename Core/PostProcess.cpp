#include "PostProcess.h"

PostProcess::PostProcess(
	ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& commandList,
	float width, float height, UINT fogSRVIndex)
{
	m_bloomLevels = 3;

	rtvSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	cbvSrvUavSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	m_bufferSize = 1 + m_bloomLevels * 2;

	CreateDescriptors(device, width, height);

	m_buffer.resize(m_bufferSize);

	m_fogSRVIndex = fogSRVIndex;

	// CopyFilter
	m_copyFilter = make_shared<ImageFilter>(device, commandList, width, height, fogSRVIndex, COPY, m_bloomLevels);
	CreateTex2D(device, m_buffer[0], static_cast<UINT>(width), static_cast<UINT>(height), 0, m_heaps, TRUE);

	// BloomDownFilter
	for (UINT i = 1; i < m_bloomLevels + 1; i++)
	{
		float div = float(pow(2, i));

		UINT newWidth = static_cast<UINT>(width / div);
		UINT newHeight = static_cast<UINT>(height / div);

		shared_ptr<ImageFilter> downfilter = make_shared<ImageFilter>(device, commandList, newWidth, newHeight, i, BLOOM_DOWN, m_bloomLevels);
		m_downFilters.push_back(downfilter);

		shared_ptr<ImageFilter> blurXfilter = make_shared<ImageFilter>(device, commandList, newWidth, newHeight, i, BLUR_X, m_bloomLevels);
		m_blurXFilters.push_back(blurXfilter);

		shared_ptr<ImageFilter> blurYfilter = make_shared<ImageFilter>(device, commandList, newWidth, newHeight, i, BLUR_Y, m_bloomLevels);
		m_blurYFilters.push_back(blurYfilter);

		CreateTex2D(device, m_buffer[i], newWidth, newHeight, i, m_heaps, FALSE);
		CreateTex2D(device, m_buffer[i + m_bloomLevels], newWidth, newHeight, i + m_bloomLevels, m_heaps, FALSE);
	}

	// BloomUpFilter
	for (UINT i = m_bloomLevels - 1; i > 0; i--)
	{
		float div = float(pow(2, i));

		UINT newWidth = static_cast<UINT>(width / div);
		UINT newHeight = static_cast<UINT>(height / div);

		shared_ptr<ImageFilter> filter = make_shared<ImageFilter>(device, commandList, newWidth, newHeight, i, BLOOM_UP, m_bloomLevels);
		m_upFilters.push_back(filter);
	}

	// CombineFilter
	m_combineFilter = make_shared<ImageFilter>(device, commandList, width, height, 0, COMBINE, m_bloomLevels);
}

PostProcess::~PostProcess()
{
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
	ComPtr<ID3D12DescriptorHeap>& srv,
	ComPtr<ID3D12DescriptorHeap>& dsv,
	UINT frameIndex)
{
	m_copyFilter->Render(commandList, m_rtv, srv);

	commandList->SetComputeRootSignature(Graphics::computeRootSignature.Get());

	ID3D12DescriptorHeap* ppHeaps[] = { m_heaps.Get() };
	commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	for (UINT i = 1; i < m_bloomLevels + 1; i++)
	{
		if (i == 1)
		{
			SetBarrier(commandList, m_buffer[i - 1],
				D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		}
		else
		{
			SetUAVBarrier(commandList, m_buffer[i - 1]);
			SetBarrier(commandList, m_buffer[i - 1],
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		}

		commandList->SetPipelineState(Graphics::samplingCSPSO.Get());
		m_downFilters[i - 1]->ComputeRender(commandList, m_heaps);

		// Blur
		SetUAVBarrier(commandList, m_buffer[i]);
		SetBarrier(commandList, m_buffer[i],
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		commandList->SetPipelineState(Graphics::blurXCSPSO.Get());
		m_blurXFilters[i - 1]->ComputeRender(commandList, m_heaps);

		SetUAVBarrier(commandList, m_buffer[i + m_bloomLevels]);
		SetBarrier(commandList, m_buffer[i + m_bloomLevels],
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		SetBarrier(commandList, m_buffer[i],
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		commandList->SetPipelineState(Graphics::blurYCSPSO.Get());
		m_blurYFilters[i - 1]->ComputeRender(commandList, m_heaps);

		SetUAVBarrier(commandList, m_buffer[i]);
		SetBarrier(commandList, m_buffer[i + m_bloomLevels],
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	}

	for (UINT i = m_bloomLevels - 1; i > 0; i--)
	{
		SetUAVBarrier(commandList, m_buffer[i + 1]);
		SetBarrier(commandList, m_buffer[i + 1],
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		SetBarrier(commandList, m_buffer[i],
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		commandList->SetPipelineState(Graphics::samplingCSPSO.Get());
		m_upFilters[m_bloomLevels - (i + 1)]->ComputeRender(commandList, m_heaps);

		// Blur
		SetUAVBarrier(commandList, m_buffer[i]);
		SetBarrier(commandList, m_buffer[i],
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		commandList->SetPipelineState(Graphics::blurXCSPSO.Get());
		m_blurXFilters[i - 1]->ComputeRender(commandList, m_heaps);

		SetUAVBarrier(commandList, m_buffer[i + m_bloomLevels]);
		SetBarrier(commandList, m_buffer[i + m_bloomLevels],
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		SetBarrier(commandList, m_buffer[i],
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		commandList->SetPipelineState(Graphics::blurYCSPSO.Get());
		m_blurYFilters[i - 1]->ComputeRender(commandList, m_heaps);

		SetUAVBarrier(commandList, m_buffer[i]);
		SetBarrier(commandList, m_buffer[i + m_bloomLevels],
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	}

	SetBarrier(commandList, m_buffer[0],
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	SetUAVBarrier(commandList, m_buffer[1]);
	SetBarrier(commandList, m_buffer[1],
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	m_combineFilter->Render(commandList, rtv, m_heaps);

	SetBarrier(commandList, m_buffer[0],
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
	SetBarrier(commandList, m_buffer[1],
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	for (UINT i = 2; i < m_bloomLevels + 1; i++)
	{
		SetBarrier(commandList, m_buffer[i],
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	}
}

void PostProcess::CreateTex2D(
	ComPtr<ID3D12Device>& device, ComPtr<ID3D12Resource>& texture,
	UINT width, UINT height, UINT index,
	ComPtr<ID3D12DescriptorHeap>& heaps, bool isPing)
{
	auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	D3D12_RESOURCE_FLAGS resFlags;
	D3D12_RESOURCE_STATES resState;
	if (isPing)
	{
		resFlags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		resState = D3D12_RESOURCE_STATE_RENDER_TARGET;
	}
	else
	{
		resFlags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		resState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	}

	auto texDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R16G16B16A16_FLOAT, // 텍스처 포맷
		width,                          // 화면 너비
		height,                         // 화면 높이
		1,                              // arraySize
		1,                              // mipLevels
		1,                              // sampleCount
		0,                              // sampleQuality
		resFlags
	);

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	clearValue.Color[0] = 0.0f;
	clearValue.Color[1] = 0.0f;
	clearValue.Color[2] = 0.0f;
	clearValue.Color[3] = 1.0f; // Alpha

	ThrowIfFailed(device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		resState,
		(isPing) ? &clearValue : nullptr,
		IID_PPV_ARGS(&texture)
	));

	if (isPing)
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtv->GetCPUDescriptorHandleForHeapStart());

		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = texture->GetDesc().Format;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

		device->CreateRenderTargetView(texture.Get(), &rtvDesc, rtvHandle);
	}
	else
	{
		// Create a UAV for each frame
		CD3DX12_CPU_DESCRIPTOR_HANDLE uavHandle(heaps->GetCPUDescriptorHandleForHeapStart(), cbvSrvUavSize * (2 * index + 1));

		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = texture->GetDesc().Format;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		uavDesc.Texture2D.MipSlice = 0;

		device->CreateUnorderedAccessView(texture.Get(), nullptr, &uavDesc, uavHandle);
	}

	// Create a SRV for each frame
	CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(heaps->GetCPUDescriptorHandleForHeapStart(), cbvSrvUavSize * (2 * index));

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
	rtvHeapDesc.NumDescriptors = 1;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtv)));

	D3D12_DESCRIPTOR_HEAP_DESC pingHeapDesc = {};
	pingHeapDesc.NumDescriptors = 2 + m_bloomLevels * 4;
	pingHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	pingHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(device->CreateDescriptorHeap(&pingHeapDesc, IID_PPV_ARGS(&m_heaps)));
}