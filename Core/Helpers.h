#pragma once

#include <d3d12.h>
#include "d3dx12.h"

#include <wrl.h>
#include <stdexcept>

#include "Vertex.h"
#include "Mesh.h"
#include "ConstantBuffers.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;

inline std::string HrToString(HRESULT hr)
{
	char s_str[64] = {};
	sprintf_s(s_str, "HRESULT of 0x%08X", static_cast<UINT>(hr));
	return std::string(s_str);
}

class HrException : public std::runtime_error
{
public:
	HrException(HRESULT hr) : std::runtime_error(HrToString(hr)), m_hr(hr) {}
	HRESULT Error() const { return m_hr; }
private:
	const HRESULT m_hr;
};

#define SAFE_RELEASE(p) if (p) (p)->Release()

inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		throw HrException(hr);
	}
}

static void CreateVertexBuffer(ComPtr<ID3D12Device>& device,
	ComPtr<ID3D12GraphicsCommandList>& commandList,
	const std::vector<Vertex>& vertcies,
	std::shared_ptr<Mesh>& mesh)
{

	const UINT vertexBufferSizeInBytes =
		static_cast<UINT>(vertcies.size() * sizeof(Vertex));

	auto buffer = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSizeInBytes);

	auto defaultHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	ThrowIfFailed(device->CreateCommittedResource(
		&defaultHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&buffer,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&mesh->vertexBuffer)));

	auto uploadHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	ThrowIfFailed(device->CreateCommittedResource(
		&uploadHeapProps,
		D3D12_HEAP_FLAG_NONE, // no flags
		&buffer,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&mesh->vertexUploadHeap)));


	D3D12_SUBRESOURCE_DATA vertexData = {};
	vertexData.pData = vertcies.data();
	vertexData.RowPitch = vertexBufferSizeInBytes;
	vertexData.SlicePitch = vertexBufferSizeInBytes;

	UpdateSubresources(commandList.Get(), mesh->vertexBuffer.Get(), mesh->vertexUploadHeap.Get(), 0, 0, 1, &vertexData);

	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		mesh->vertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	commandList->ResourceBarrier(1, &barrier);

	mesh->vertexBufferView.BufferLocation = mesh->vertexBuffer->GetGPUVirtualAddress();
	mesh->vertexBufferView.StrideInBytes = sizeof(Vertex);
	mesh->vertexBufferView.SizeInBytes = vertexBufferSizeInBytes;

}

static void CreateIndexBuffer(ComPtr<ID3D12Device>& device,
	ComPtr<ID3D12GraphicsCommandList>& commandList,
	const std::vector<uint32_t>& indices,
	std::shared_ptr<Mesh>& mesh)
{
	mesh->indexBufferCount = indices.size();

	const UINT indexBufferSizeInBytes =
		static_cast<UINT>(indices.size() * sizeof(uint32_t));

	auto defaultHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	auto buffer = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSizeInBytes);
	ThrowIfFailed(device->CreateCommittedResource(
		&defaultHeapProps,
		D3D12_HEAP_FLAG_NONE, // no flags
		&buffer,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&mesh->indexBuffer)));

	auto uploadHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	ThrowIfFailed(device->CreateCommittedResource(
		&uploadHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&buffer,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&mesh->indexUploadHeap)));


	D3D12_SUBRESOURCE_DATA indexData = {};
	indexData.pData = indices.data();
	indexData.RowPitch = indexBufferSizeInBytes;
	indexData.SlicePitch = indexBufferSizeInBytes;

	UpdateSubresources(commandList.Get(), mesh->indexBuffer.Get(), mesh->indexUploadHeap.Get(), 0, 0, 1, &indexData);

	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		mesh->indexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);
	commandList->ResourceBarrier(1, &barrier);

	mesh->indexBufferView.BufferLocation = mesh->indexBuffer->GetGPUVirtualAddress();
	mesh->indexBufferView.Format = DXGI_FORMAT_R32_UINT;
	mesh->indexBufferView.SizeInBytes = indexBufferSizeInBytes;

}

static void CreateConstBuffer(ComPtr<ID3D12Device>& device,
	ComPtr<ID3D12GraphicsCommandList>& commandList,
	ComPtr<ID3D12Resource>& meshConstsUploadHeap,
	MeshConstants& meshConstsBufferData,
	UINT8*& meshConstsBufferDataBegin,
	CD3DX12_CPU_DESCRIPTOR_HANDLE basicHandle)
{
	const UINT meshConstantsSize = sizeof(GlobalConstants);

	auto uploadHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto buffer = CD3DX12_RESOURCE_DESC::Buffer(meshConstantsSize);
	ThrowIfFailed(device->CreateCommittedResource(
		&uploadHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&buffer,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&meshConstsUploadHeap)));

	CD3DX12_RANGE readRange(0, 0);
	ThrowIfFailed(meshConstsUploadHeap->Map(0, &readRange, reinterpret_cast<void**>(&meshConstsBufferDataBegin)));

	memcpy(meshConstsBufferDataBegin, &meshConstsBufferData, sizeof(meshConstsBufferData));

	// Describe and create a constant buffer view.
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = meshConstsUploadHeap->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = (meshConstantsSize + 255) & ~255; // 256-byte 정렬
	device->CreateConstantBufferView(&cbvDesc, basicHandle);

	basicHandle.Offset(1, device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
}

inline std::wstring StringToWString(const std::string& str) {
	return std::wstring(str.begin(), str.end());
}

static void CreateTextureBuffer(ComPtr<ID3D12Device>& device,
	ComPtr<ID3D12GraphicsCommandList>& commandList,
	ComPtr<ID3D12Resource>& texture,
	ComPtr<ID3D12Resource>& textureUploadHeap,
	CD3DX12_CPU_DESCRIPTOR_HANDLE basicHandle,
	const std::string &filename)
{
	auto image = std::make_unique<ScratchImage>();

	// std::string → std::wstring 변환
	std::wstring wideFilename = StringToWString(filename);

	ThrowIfFailed(DirectX::LoadFromWICFile(wideFilename.c_str(), DirectX::WIC_FLAGS_NONE, nullptr, *image));

	DirectX::TexMetadata metaData = image.get()->GetMetadata();
	ThrowIfFailed(CreateTexture(device.Get(), metaData, &texture));

	std::vector<D3D12_SUBRESOURCE_DATA> subresources;
	ThrowIfFailed(PrepareUpload(device.Get(), image.get()->GetImages(), image.get()->GetImageCount(), metaData, subresources));

	// upload is implemented by application developer. Here's one solution using <d3dx12.h>
	const UINT64 uploadBufferSize = GetRequiredIntermediateSize(texture.Get(), 0, static_cast<unsigned int>(subresources.size()));

	auto uploadHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto buffer = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
	ThrowIfFailed(device->CreateCommittedResource(
		&uploadHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&buffer,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&textureUploadHeap)));

	UpdateSubresources(
		commandList.Get(), texture.Get(), textureUploadHeap.Get(),
		0, 0, static_cast<unsigned int>(subresources.size()), subresources.data());

	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		texture.Get(), // 텍스처 리소스
		D3D12_RESOURCE_STATE_COPY_DEST, // 이전 상태
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE // 새로운 상태
	);
	commandList->ResourceBarrier(1, &barrier);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = metaData.format; // 텍스처의 포맷
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = static_cast<UINT>(metaData.mipLevels);

	device->CreateShaderResourceView(
		texture.Get(), // 텍스처 리소스
		&srvDesc, // SRV 설명
		basicHandle // 디스크립터 힙의 핸들
	);
}