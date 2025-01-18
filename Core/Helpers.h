#pragma once

#undef max

#include <d3d12.h>
#include <dxgi1_6.h>
#include "d3dx12.h"

#include <wrl.h>
#include <stdexcept>

#include "Vertex.h"
#include "Mesh.h"
#include "ConstantBuffers.h"

#include <iostream>
#include <cmath>
#include <algorithm>
#include <string>
#include <unordered_map>

#include "DirectXTex.h"
#include "directxtk12\DDSTextureLoader.h"
#include "directxtk12\ResourceUploadBatch.h"

using namespace std;
using Microsoft::WRL::ComPtr;
using namespace DirectX;

struct GuiState {
	bool isDrawNormals = false;
	bool isWireframe = false;
	bool isUseTextrue = true;
	bool isMeshChanged = false;
	string changedMeshKey = "";
};

struct DirtyFlag {
	bool isPostProcessFlag = false;
};

inline string HrToString(HRESULT hr)
{
	char s_str[64] = {};
	sprintf_s(s_str, "HRESULT of 0x%08X", static_cast<UINT>(hr));
	return string(s_str);
}

class HrException : public runtime_error
{
public:
	HrException(HRESULT hr) : runtime_error(HrToString(hr)), m_hr(hr) {}
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

static void SetBarrier(
	ComPtr<ID3D12GraphicsCommandList>& commandList,
	ComPtr<ID3D12Resource>& buffer,
	D3D12_RESOURCE_STATES stateBefore,
	D3D12_RESOURCE_STATES stateAfter
)
{
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		buffer.Get(),
		stateBefore, stateAfter);
	commandList->ResourceBarrier(1, &barrier);
}

static void CreateVertexBuffer(
	ComPtr<ID3D12Device>& device,
	ComPtr<ID3D12GraphicsCommandList>& commandList,
	const vector<Vertex>& vertices,
	shared_ptr<Mesh>& mesh)
{
	mesh->vertexBufferCount = static_cast<UINT>(vertices.size());

	const UINT vertexBufferSizeInBytes =
		static_cast<UINT>(vertices.size() * sizeof(Vertex));

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
	vertexData.pData = vertices.data();
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

static void CreateIndexBuffer(
	ComPtr<ID3D12Device>& device,
	ComPtr<ID3D12GraphicsCommandList>& commandList,
	const vector<uint32_t>& indices,
	shared_ptr<Mesh>& mesh)
{
	mesh->indexBufferCount = static_cast<UINT>(indices.size());

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

template <typename T>
static void CreateConstUploadBuffer(
	ComPtr<ID3D12Device>& device,
	ComPtr<ID3D12GraphicsCommandList>& commandList,
	ComPtr<ID3D12Resource>& constsUploadHeap,
	T& constsBufferData,
	UINT8*& constsBufferDataBegin)
{
	const UINT constantsSize = sizeof(T);

	auto uploadHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto buffer = CD3DX12_RESOURCE_DESC::Buffer(constantsSize);
	ThrowIfFailed(device->CreateCommittedResource(
		&uploadHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&buffer,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&constsUploadHeap)));

	CD3DX12_RANGE readRange(0, 0);
	ThrowIfFailed(constsUploadHeap->Map(0, &readRange, reinterpret_cast<void**>(&constsBufferDataBegin)));

	memcpy(constsBufferDataBegin, &constsBufferData, sizeof(constsBufferData));
}

inline wstring StringToWString(const string& str) {
	return wstring(str.begin(), str.end());
}


static void CreateDDSTextureBuffer(
	ComPtr<ID3D12Device>& device,
	ComPtr<ID3D12CommandQueue>& commandQueue,
	const string& filename,
	shared_ptr<Mesh>& newMesh,
	CD3DX12_CPU_DESCRIPTOR_HANDLE textureHandle,
	vector<ComPtr<ID3D12Resource>>& textures,
	UINT& textureCnt,
	unordered_map<string, int>& textureIdx,
	CubemapIndexConstants& cubemapIndexConstsBufferData)
{
	if (textureIdx.find(filename) != textureIdx.end())
	{

		if (filename.find("ambient") != std::string::npos)
			cubemapIndexConstsBufferData.cubemapAmbientIndex = textureCnt;
		else if (filename.find("diffuse") != std::string::npos)
			cubemapIndexConstsBufferData.cubemapDiffuseIndex = textureCnt;
		else if (filename.find("specular") != std::string::npos)
			cubemapIndexConstsBufferData.cubemapSpecularIndex = textureCnt;

		return;
	}

	UINT size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	textureHandle.Offset(size * textureCnt);

	wstring wideFilename = StringToWString(filename);

	// ResourceUploadBatch 객체 생성
	ResourceUploadBatch resourceUpload(device.Get());
	resourceUpload.Begin();

	// DDS 텍스처 로드
	ComPtr<ID3D12Resource> tex;
	DDS_ALPHA_MODE alphaMode;
	bool isCubeMap = true;

	ThrowIfFailed(CreateDDSTextureFromFileEx(
		device.Get(),
		resourceUpload,
		wideFilename.c_str(),
		0,
		D3D12_RESOURCE_FLAG_NONE,
		DDS_LOADER_DEFAULT,
		tex.GetAddressOf(),
		&alphaMode,
		&isCubeMap));

	// 업로드 배치 종료 및 GPU에 제출
	auto uploadFuture = resourceUpload.End(commandQueue.Get());
	uploadFuture.wait();

	// SRV 생성
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = tex->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.Texture2D.MipLevels = tex->GetDesc().MipLevels;

	device->CreateShaderResourceView(
		tex.Get(),
		&srvDesc,
		textureHandle
	);

	// 리소스 관리
	textures.push_back(tex);

	if (filename.find("ambient") != std::string::npos)
		cubemapIndexConstsBufferData.cubemapAmbientIndex = textureCnt;
	else if (filename.find("diffuse") != std::string::npos)
		cubemapIndexConstsBufferData.cubemapDiffuseIndex = textureCnt;
	else if (filename.find("specular") != std::string::npos)
		cubemapIndexConstsBufferData.cubemapSpecularIndex = textureCnt;
	else
		assert(false && "Texture file does not exist!");

	textureIdx.insert({ filename, textureCnt });
	textureCnt++;

	wprintf(L"Successfully loaded DDS texture: %s, location is %d\n", wideFilename.c_str(), textureCnt - 1);
}

static void CreateMipMapTextureBuffer(
	ComPtr<ID3D12Device>& device,
	ComPtr<ID3D12GraphicsCommandList>& commandList,
	const string& filename,
	shared_ptr<Mesh>& newMesh,
	CD3DX12_CPU_DESCRIPTOR_HANDLE textureHandle,
	vector<ComPtr<ID3D12Resource>>& textures,
	vector<ComPtr<ID3D12Resource>>& texturesUploadHeap,
	UINT& textureCnt,
	unordered_map<string, int>& textureIdx)
{

	if (textureIdx.find(filename) != textureIdx.end())
	{
		if (filename.find("ambient") != std::string::npos)
			newMesh->constsBufferData.ambientIndex = textureCnt;
		else if (filename.find("diffuse") != std::string::npos)
			newMesh->constsBufferData.diffuseIndex = textureCnt;
		else if (filename.find("specular") != std::string::npos)
			newMesh->constsBufferData.specularIndex = textureCnt;

		return;
	}

	UINT size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	textureHandle.Offset(size * textureCnt);

	auto image = make_unique<ScratchImage>();

	wstring wideFilename = StringToWString(filename);
	ThrowIfFailed(DirectX::LoadFromWICFile(wideFilename.c_str(), DirectX::WIC_FLAGS_NONE, nullptr, *image));

	DirectX::TexMetadata metaData = image.get()->GetMetadata();

	// 최대 밉맵 레벨 계산
	size_t minLevel = 1;
	UINT mipLevels = 1 + static_cast<UINT>(
		std::floor(std::log2(
			std::max({ metaData.width, metaData.height, minLevel }))));

	std::unique_ptr<ScratchImage> mipChain = std::make_unique<ScratchImage>();
	ThrowIfFailed(DirectX::GenerateMipMaps(
		image->GetImages(),         // 입력 이미지
		image->GetImageCount(),     // 이미지 개수
		image->GetMetadata(),       // 메타데이터
		TEX_FILTER_DEFAULT,         // 필터링 방식
		mipLevels,                  // 밉맵 레벨 수
		*mipChain                   // 출력 ScratchImage
	));

	ComPtr<ID3D12Resource> texture;
	auto defaultHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	D3D12_RESOURCE_DESC textureDesc = {};
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	textureDesc.Width = static_cast<UINT>(metaData.width);
	textureDesc.Height = static_cast<UINT>(metaData.height);
	textureDesc.DepthOrArraySize = static_cast<UINT16>(metaData.arraySize);
	textureDesc.MipLevels = static_cast<UINT16>(mipLevels);
	textureDesc.Format = metaData.format;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	ThrowIfFailed(device->CreateCommittedResource(
		&defaultHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&textureDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&texture)
	));

	std::vector<D3D12_SUBRESOURCE_DATA> subresources;
	for (size_t i = 0; i < mipChain->GetImageCount(); ++i) {
		const Image* mipImage = mipChain->GetImages() + i;

		D3D12_SUBRESOURCE_DATA subresource = {};
		subresource.pData = mipImage->pixels;
		subresource.RowPitch = mipImage->rowPitch;
		subresource.SlicePitch = mipImage->slicePitch;

		subresources.push_back(subresource);
	}

	// upload is implemented by application developer. Here's one solution using <d3dx12.h>
	const UINT64 uploadBufferSize = GetRequiredIntermediateSize(texture.Get(), 0, static_cast<unsigned int>(subresources.size()));

	auto uploadHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto uploadHeapdesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
	ComPtr<ID3D12Resource> texUploadHeap;
	ThrowIfFailed(device->CreateCommittedResource(
		&uploadHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&uploadHeapdesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&texUploadHeap)));

	UpdateSubresources(
		commandList.Get(), texture.Get(), texUploadHeap.Get(),
		0, 0, static_cast<unsigned int>(subresources.size()), subresources.data());

	SetBarrier(commandList, texture,
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = metaData.format; // 텍스처의 포맷
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = mipLevels;
	srvDesc.Texture2D.MostDetailedMip = 0;

	device->CreateShaderResourceView(
		texture.Get(), // 텍스처 리소스
		&srvDesc, // SRV 설명
		textureHandle // 디스크립터 힙의 핸들
	);

	textures.push_back(texture);
	texturesUploadHeap.push_back(texUploadHeap);

	if (filename.find("ambient") != std::string::npos)
		newMesh->constsBufferData.ambientIndex = textureCnt;
	else if (filename.find("diffuse") != std::string::npos)
		newMesh->constsBufferData.diffuseIndex = textureCnt;
	else if (filename.find("specular") != std::string::npos)
		newMesh->constsBufferData.specularIndex = textureCnt;
	else
		assert(false && "Texture file does not exist!");

	textureIdx.insert({ filename, textureCnt });
	textureCnt++;

	wprintf(L"Successfully loaded MipMap texture: %s, location is %d\n", wideFilename.c_str(), textureCnt - 1);

}

static void CreateTextureBuffer(
	ComPtr<ID3D12Device>& device,
	ComPtr<ID3D12GraphicsCommandList>& commandList,
	const string& filename,
	shared_ptr<Mesh>& newMesh,
	CD3DX12_CPU_DESCRIPTOR_HANDLE textureHandle,
	vector<ComPtr<ID3D12Resource>>& textures,
	vector<ComPtr<ID3D12Resource>>& texturesUploadHeap,
	UINT& textureCnt,
	unordered_map<string, int>& textureIdx)
{
	if (textureIdx.find(filename) != textureIdx.end())
	{
		if (filename.find("ambient") != std::string::npos)
			newMesh->constsBufferData.ambientIndex = textureCnt;
		else if (filename.find("diffuse") != std::string::npos)
			newMesh->constsBufferData.diffuseIndex = textureCnt;
		else if (filename.find("specular") != std::string::npos)
			newMesh->constsBufferData.specularIndex = textureCnt;

		return;
	}

	UINT size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	textureHandle.Offset(size * textureCnt);

	auto image = make_unique<ScratchImage>();

	ComPtr<ID3D12Resource> texture;

	wstring wideFilename = StringToWString(filename);
	ThrowIfFailed(DirectX::LoadFromWICFile(wideFilename.c_str(), DirectX::WIC_FLAGS_NONE, nullptr, *image));

	DirectX::TexMetadata metaData = image.get()->GetMetadata();
	ThrowIfFailed(CreateTexture(device.Get(), metaData, &texture));

	vector<D3D12_SUBRESOURCE_DATA> subresources;
	ThrowIfFailed(PrepareUpload(device.Get(), image.get()->GetImages(), image.get()->GetImageCount(), metaData, subresources));

	// upload is implemented by application developer. Here's one solution using <d3dx12.h>
	const UINT64 uploadBufferSize = GetRequiredIntermediateSize(texture.Get(), 0, static_cast<unsigned int>(subresources.size()));

	auto uploadHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto uploadHeapdesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
	ComPtr<ID3D12Resource> texUploadHeap;
	ThrowIfFailed(device->CreateCommittedResource(
		&uploadHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&uploadHeapdesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&texUploadHeap)));

	UpdateSubresources(
		commandList.Get(), texture.Get(), texUploadHeap.Get(),
		0, 0, static_cast<unsigned int>(subresources.size()), subresources.data());

	SetBarrier(commandList, texture,
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = metaData.format; // 텍스처의 포맷
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	device->CreateShaderResourceView(
		texture.Get(), // 텍스처 리소스
		&srvDesc, // SRV 설명
		textureHandle // 디스크립터 힙의 핸들
	);

	textures.push_back(texture);
	texturesUploadHeap.push_back(texUploadHeap);

	if (filename.find("ambient") != std::string::npos)
		newMesh->constsBufferData.ambientIndex = textureCnt;
	else if (filename.find("diffuse") != std::string::npos)
		newMesh->constsBufferData.diffuseIndex = textureCnt;
	else if (filename.find("specular") != std::string::npos)
		newMesh->constsBufferData.specularIndex = textureCnt;
	else
		assert(false && "Texture file does not exist!");

	textureIdx.insert({ filename, textureCnt });
	textureCnt++;

	wprintf(L"Successfully loaded texture: %s, location is %d\n", wideFilename.c_str(), textureCnt - 1);

}

static void CreateConstDefaultBuffer(ComPtr<ID3D12Device>& device,
	ComPtr<ID3D12GraphicsCommandList>& commandList,
	shared_ptr<Mesh>& mesh)
{
	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	const UINT constantsSize = sizeof(TextureIndexConstants);

	D3D12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(constantsSize);

	ThrowIfFailed(device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&bufferDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&mesh->constsBuffer)));

	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
	ThrowIfFailed(device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&bufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&mesh->constsUploadHeap)));

	CD3DX12_RANGE readRange(0, 0);
	ThrowIfFailed(mesh->constsUploadHeap->Map(0, &readRange, reinterpret_cast<void**>(&mesh->constsBufferDataBegin)));
	memcpy(mesh->constsBufferDataBegin, &mesh->constsBufferData, sizeof(mesh->constsBufferData));
	mesh->constsUploadHeap->Unmap(0, nullptr);

	commandList->CopyBufferRegion(mesh->constsBuffer.Get(), 0, mesh->constsUploadHeap.Get(), 0, sizeof(mesh->constsBufferData));

	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		mesh->constsBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
	);
	commandList->ResourceBarrier(1, &barrier);
}

static void CreateEmptyTexture(
	ComPtr<ID3D12Device>& device,
	ComPtr<ID3D12GraphicsCommandList>& commandList,
	CD3DX12_CPU_DESCRIPTOR_HANDLE textureHandle,
	vector<ComPtr<ID3D12Resource>>& textures,
	vector<ComPtr<ID3D12Resource>>& texturesUploadHeap,
	UINT& textureCnt)
{
	ComPtr<ID3D12Resource> texture;

	// 1x1 검정 텍스처 데이터 (RGBA)
	unsigned char blackPixel[4] = { 255, 255, 255, 255 };

	// 텍스처 리소스 생성 (GPU 기본 힙)
	auto textureDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R8G8B8A8_UNORM,
		1, 1, 1, 0,
		1, 0,
		D3D12_RESOURCE_FLAG_NONE
	);

	auto defaultHeapDesc = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	ThrowIfFailed(device->CreateCommittedResource(
		&defaultHeapDesc,
		D3D12_HEAP_FLAG_NONE,
		&textureDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&texture)
	));

	// 업로드 힙 리소스 생성
	auto uploadHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	const UINT64 uploadBufferSize = GetRequiredIntermediateSize(texture.Get(), 0, 1);
	auto buffer = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
	ComPtr<ID3D12Resource> textureUploadHeap;
	ThrowIfFailed(device->CreateCommittedResource(
		&uploadHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&buffer,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&textureUploadHeap)));

	// 텍스처 데이터 복사
	D3D12_SUBRESOURCE_DATA textureData = {};
	textureData.pData = blackPixel;
	textureData.RowPitch = 4; // 1x1 픽셀, RGBA
	textureData.SlicePitch = textureData.RowPitch;

	UpdateSubresources(commandList.Get(), texture.Get(), textureUploadHeap.Get(), 0, 0, 1, &textureData);

	SetBarrier(commandList, texture,
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = texture->GetDesc().Format; // 텍스처의 포맷
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	UINT size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	device->CreateShaderResourceView(
		texture.Get(), // 텍스처 리소스
		&srvDesc, // SRV 설명
		textureHandle // 디스크립터 힙의 핸들
	);

	textures.push_back(texture);
	texturesUploadHeap.push_back(textureUploadHeap);
	textureCnt++;
}