#include "ImageFilter.h"

ImageFilter::ImageFilter(
	ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& commandList,
	UINT width, UINT height, UINT index, FilterOption option, UINT bloomLevels)
{
	cbvSrvUavSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	m_samplingConstsBufferData.dx = 1.0f / width;
	m_samplingConstsBufferData.dy = 1.0f / height;
	
	m_width = width;
	m_height = height;

	m_option = option;
	switch (m_option)
	{
	case COPY:
		break;
	case BLOOM_DOWN:
		srvIndex = 2 * index - 2;
		uavIndex = 2 * index + 1;
		tgx = UINT(ceil(m_width / 32.0f));
		tgy = UINT(ceil(m_height / 32.0f));
		break;
	case BLOOM_UP:
		srvIndex = 2 * index + 2;
		uavIndex = 2 * index + 1;
		tgx = UINT(ceil(m_width / 32.0f));
		tgy = UINT(ceil(m_height / 32.0f));
		break;
	case BLUR_X:
		srvIndex = 2 * index;
		uavIndex = 2 * (index + bloomLevels) + 1;
		tgx = UINT(ceil(m_width / 256.0f));
		tgy = m_height;
		break;
	case BLUR_Y:
		srvIndex = 2 * (index + bloomLevels);
		uavIndex = 2 * index + 1;
		tgx = m_width;
		tgy = UINT(ceil(m_height / 256.0f));
		break;
	case COMBINE:
		break;
	default:
		break;
	}

	m_samplingConstsBufferData.index = index;

	CreateConstUploadBuffer(
		device, m_samplingConstsUploadHeap, m_samplingConstsBufferData, m_samplingConstsBufferDataBegin);
}

void ImageFilter::Update(SamplingConstants& m_combineConsts)
{
	m_samplingConstsBufferData.strength = m_combineConsts.strength;
	m_samplingConstsBufferData.exposure = m_combineConsts.exposure;
	m_samplingConstsBufferData.gamma = m_combineConsts.gamma;
	memcpy(m_samplingConstsBufferDataBegin, &m_samplingConstsBufferData, sizeof(m_samplingConstsBufferData));
}

void ImageFilter::Render(
	ComPtr<ID3D12GraphicsCommandList>& commandList,
	ComPtr<ID3D12DescriptorHeap>& rtv,
	ComPtr<ID3D12DescriptorHeap>& srv)
{
	if (m_option == COPY)
	{
		commandList->SetPipelineState(Graphics::samplingPSO.Get());

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtv->GetCPUDescriptorHandleForHeapStart());
		commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

		CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle(srv->GetGPUDescriptorHandleForHeapStart(), cbvSrvUavSize * m_samplingConstsBufferData.index);
		commandList->SetGraphicsRootDescriptorTable(5, srvHandle);

		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList->DrawInstanced(3, 1, 0, 0);

	}
	else if (m_option == COMBINE)
	{
		commandList->SetPipelineState(Graphics::combinePSO.Get());

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtv->GetCPUDescriptorHandleForHeapStart());
		commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

		commandList->SetGraphicsRootConstantBufferView(4, m_samplingConstsUploadHeap->GetGPUVirtualAddress());

		CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle(srv->GetGPUDescriptorHandleForHeapStart());
		commandList->SetGraphicsRootDescriptorTable(5, srvHandle);

		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList->DrawInstanced(3, 1, 0, 0);
	}
}

void ImageFilter::ComputeRender(
	ComPtr<ID3D12GraphicsCommandList>& commandList,
	ComPtr<ID3D12DescriptorHeap>& heaps)
{
	CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle(heaps->GetGPUDescriptorHandleForHeapStart(), cbvSrvUavSize * srvIndex);
	commandList->SetComputeRootDescriptorTable(0, srvHandle);

	CD3DX12_GPU_DESCRIPTOR_HANDLE uavHandle(heaps->GetGPUDescriptorHandleForHeapStart(), cbvSrvUavSize * uavIndex);
	commandList->SetComputeRootDescriptorTable(1, uavHandle);

	commandList->Dispatch(tgx, tgy, 1);
}