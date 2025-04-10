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
#include <filesystem>

#include "DirectXTex.h"
#include "directxtk12\DDSTextureLoader.h"
#include "directxtk12\ResourceUploadBatch.h"

#include "DirectXTexEXR.h"

static const UINT FrameCount = 3;

using namespace std;
using Microsoft::WRL::ComPtr;
using namespace DirectX;

struct TextureInfo {
	UINT heapIndex;
	string path;
	string filename;
	string ext;
	ComPtr<ID3D12Resource> texture;
	ComPtr<ID3D12Resource> textureUploadHeap;
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle;
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle;
};

struct GuiState {
	string changedMeshKey = "";
	bool isDrawNormals = false;
	bool isWireframe = false;
	bool isMeshChanged = false;
	bool isLightChanged = false;
	bool isEnvChanged = false;
	bool isMirrorEnabled = false;
	bool isMirrorChanged = false;
	bool isPostEffectsEnabled = false;
	bool isPostEffectsChanged = false;
	bool isPostProcessEnabled = true;
	bool isPostProcessChanged = false;
};

struct DirtyFlag {
};

struct ShapesInfo
{
	UINT sphereNum = 0;
	UINT sphereCnt = 0;
	UINT squareNum = 0;
	UINT squareCnt = 0;
	UINT boxNum = 0;
	UINT boxCnt = 0;
	UINT testBeforeCnt = 0;
	UINT testAfterCnt = 0;
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

static void SetUAVBarrier(
	ComPtr<ID3D12GraphicsCommandList>& commandList,
	ComPtr<ID3D12Resource>& buffer)
{
	D3D12_RESOURCE_BARRIER uavBarrier = CD3DX12_RESOURCE_BARRIER::UAV(buffer.Get());
	commandList->ResourceBarrier(1, &uavBarrier);
}

static void CreateBuffer(
	ComPtr<ID3D12Device>& device, ComPtr<ID3D12Resource>& buffer,
	UINT width, UINT height, UINT sampleCount,
	DXGI_FORMAT format, D3D12_SRV_DIMENSION srvDimension, D3D12_RESOURCE_STATES initState,
	ComPtr<ID3D12DescriptorHeap> rtvHeap, UINT rtvIndex, ComPtr<ID3D12DescriptorHeap> srvHeap, UINT srvIndex)
{
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msaaLevels = {};
	msaaLevels.Format = format;
	msaaLevels.SampleCount = sampleCount;
	msaaLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msaaLevels.NumQualityLevels = 0;

	ThrowIfFailed(device->CheckFeatureSupport(
		D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
		&msaaLevels,
		sizeof(msaaLevels)
	));

	if (msaaLevels.NumQualityLevels == 0) {
		throw std::runtime_error("The specified sample count is not supported for the given format.");
	}

	auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	auto texDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		format, // 텍스처 포맷
		width,                          // 화면 너비
		height,                         // 화면 높이
		1,                              // arraySize
		1,                              // mipLevels
		sampleCount,                    // sampleCount
		0,                              // sampleQuality
		D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET // RT로 사용할 예정이면 플래그 설정
	);

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = format; // 텍스처의 포맷
	clearValue.Color[0] = 0.0f; // Red
	clearValue.Color[1] = 0.0f; // Green
	clearValue.Color[2] = 0.0f; // Blue
	clearValue.Color[3] = 1.0f; // Alpha

	ThrowIfFailed(device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		initState,
		&clearValue,
		IID_PPV_ARGS(&buffer)
	));

	UINT rtvSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	UINT srvSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	if (rtvHeap)
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart(), rtvSize * rtvIndex);

		// Create a RTV for each frame
		device->CreateRenderTargetView(buffer.Get(), nullptr, rtvHandle);
	}

	if (srvHeap)
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(srvHeap->GetCPUDescriptorHandleForHeapStart(), srvSize * srvIndex);
		// Create a SRV for each frame
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = format;
		srvDesc.ViewDimension = srvDimension;
		srvDesc.Texture2D.MipLevels = buffer->GetDesc().MipLevels;
		srvDesc.Texture2DMS.UnusedField_NothingToDefine = 0; // 필드 없음

		device->CreateShaderResourceView(
			buffer.Get(),
			&srvDesc,
			srvHandle
		);
	}
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
		D3D12_RESOURCE_STATE_COMMON,
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

	SetBarrier(commandList, mesh->vertexBuffer,
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

	UpdateSubresources(commandList.Get(), mesh->vertexBuffer.Get(), mesh->vertexUploadHeap.Get(), 0, 0, 1, &vertexData);

	SetBarrier(commandList, mesh->vertexBuffer,
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

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
		D3D12_RESOURCE_STATE_COMMON,
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

	SetBarrier(commandList, mesh->indexBuffer,
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

	UpdateSubresources(commandList.Get(), mesh->indexBuffer.Get(), mesh->indexUploadHeap.Get(), 0, 0, 1, &indexData);

	SetBarrier(commandList, mesh->indexBuffer,
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);

	mesh->indexBufferView.BufferLocation = mesh->indexBuffer->GetGPUVirtualAddress();
	mesh->indexBufferView.Format = DXGI_FORMAT_R32_UINT;
	mesh->indexBufferView.SizeInBytes = indexBufferSizeInBytes;

}

template <typename T>
static void CreateConstUploadBuffer(
	ComPtr<ID3D12Device>& device,
	ComPtr<ID3D12Resource>& constsUploadHeap,
	T& constsBufferData,
	UINT8*& constsBufferDataBegin)
{
	const UINT constantsSize = (sizeof(T) + 255) & ~255;

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
	const string& filepath,
	const string& lowerFilepath,
	shared_ptr<Mesh>& newMesh,
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle,
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle,
	UINT& textureIndex,
	unordered_map<string, TextureInfo>& textureInfos,
	CubemapIndexConstants& cubemapIndexConstsBufferData,
	bool isCubeMap)
{
	UINT size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	cpuHandle.Offset(size * textureIndex);
	gpuHandle.Offset(size * textureIndex);

	wstring wideFilename = StringToWString(filepath);

	// ResourceUploadBatch 객체 생성
	ResourceUploadBatch resourceUpload(device.Get());
	resourceUpload.Begin();

	// DDS 텍스처 로드
	ComPtr<ID3D12Resource> texture;
	DDS_ALPHA_MODE alphaMode;

	ThrowIfFailed(CreateDDSTextureFromFileEx(
		device.Get(),
		resourceUpload,
		wideFilename.c_str(),
		0,
		D3D12_RESOURCE_FLAG_NONE,
		DDS_LOADER_DEFAULT,
		texture.GetAddressOf(),
		&alphaMode,
		&isCubeMap));

	// 업로드 배치 종료 및 GPU에 제출
	auto uploadFuture = resourceUpload.End(commandQueue.Get());
	uploadFuture.wait();

	// SRV 생성
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = texture->GetDesc().Format;
	srvDesc.ViewDimension = isCubeMap ? D3D12_SRV_DIMENSION_TEXTURECUBE : D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = texture->GetDesc().MipLevels;

	device->CreateShaderResourceView(
		texture.Get(),
		&srvDesc,
		cpuHandle
	);

	if (lowerFilepath.find("env") != std::string::npos)
		cubemapIndexConstsBufferData.cubemapEnvIndex = textureIndex;
	else if (lowerFilepath.find("diffuse") != std::string::npos)
		cubemapIndexConstsBufferData.cubemapDiffuseIndex = textureIndex;
	else if (lowerFilepath.find("specular") != std::string::npos)
		cubemapIndexConstsBufferData.cubemapSpecularIndex = textureIndex;
	else if (lowerFilepath.find("brdf") != std::string::npos)
		cubemapIndexConstsBufferData.brdfIndex = textureIndex;
	else
		assert(false && "DDS Texture file does not exist!");

	std::filesystem::path filePath(filepath);

	std::string filename = filePath.stem().string();  // 확장자 제외한 파일명
	std::string extension = filePath.extension().string();  // 확장자
	std::string filenameExtension = filePath.filename().string();

	textureInfos.insert({ filenameExtension, TextureInfo{
		textureIndex, filepath, filename, extension,
		texture, nullptr, cpuHandle, gpuHandle } });
	textureIndex++;

	wprintf(L"Successfully loaded DDS texture: %s, location is %d\n", wideFilename.c_str(), textureIndex - 1);
}

static void CreateEXRTextureBuffer(
	ComPtr<ID3D12Device>& device,
	ComPtr<ID3D12GraphicsCommandList>& commandList,
	const string& filepath,
	const string& lowerFilepath,
	shared_ptr<Mesh>& newMesh,
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle,
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle,
	UINT& textureIndex,
	unordered_map<string, TextureInfo>& textureInfos)
{
	UINT size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	cpuHandle.Offset(size * textureIndex);
	gpuHandle.Offset(size * textureIndex);

	ComPtr<ID3D12Resource> texture;

	wstring wideFilename = StringToWString(filepath);
	DirectX::TexMetadata metaData;
	auto image = make_unique<ScratchImage>();
	ThrowIfFailed(LoadFromEXRFile(wideFilename.c_str(), &metaData, *image));

	ThrowIfFailed(CreateTexture(device.Get(), metaData, &texture));

	vector<D3D12_SUBRESOURCE_DATA> subresources;
	ThrowIfFailed(PrepareUpload(
		device.Get(),
		image.get()->GetImages(),
		image.get()->GetImageCount(),
		metaData,
		subresources));

	// upload is implemented by application developer. Here's one solution using <d3dx12.h>
	const UINT64 uploadBufferSize = GetRequiredIntermediateSize(texture.Get(), 0, static_cast<unsigned int>(subresources.size()));

	auto uploadHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto uploadHeapdesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
	ComPtr<ID3D12Resource> textureUploadHeap;
	ThrowIfFailed(device->CreateCommittedResource(
		&uploadHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&uploadHeapdesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&textureUploadHeap)));

	UpdateSubresources(
		commandList.Get(), texture.Get(), textureUploadHeap.Get(),
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
		cpuHandle // 디스크립터 힙의 핸들
	);

	if (lowerFilepath.find("albedo") != std::string::npos)
		newMesh->textureIndexConstsBufferData.albedoIndex = textureIndex;
	else if (lowerFilepath.find("diffuse") != std::string::npos)
		newMesh->textureIndexConstsBufferData.diffuseIndex = textureIndex;
	else if (lowerFilepath.find("specular") != std::string::npos)
		newMesh->textureIndexConstsBufferData.specularIndex = textureIndex;
	else if (lowerFilepath.find("normal") != std::string::npos)
		newMesh->textureIndexConstsBufferData.normalIndex = textureIndex;
	else if (lowerFilepath.find("height") != std::string::npos)
		newMesh->textureIndexConstsBufferData.heightIndex = textureIndex;
	else if (lowerFilepath.find("ao") != std::string::npos)
		newMesh->textureIndexConstsBufferData.aoIndex = textureIndex;
	else if (lowerFilepath.find("metallic") != std::string::npos)
		newMesh->textureIndexConstsBufferData.metallicIndex = textureIndex;
	else if (lowerFilepath.find("roughness") != std::string::npos)
		newMesh->textureIndexConstsBufferData.roughnessIndex = textureIndex;
	else if (lowerFilepath.find("emissive") != std::string::npos)
		newMesh->textureIndexConstsBufferData.emissiveIndex = textureIndex;
	else
		assert(false && "EXR Texture file does not exist!");

	std::filesystem::path filePath(filepath);

	std::string filename = filePath.stem().string();  // 확장자 제외한 파일명
	std::string extension = filePath.extension().string();  // 확장자
	std::string filenameExtension = filePath.filename().string();

	textureInfos.insert({ filenameExtension, TextureInfo{
		textureIndex, filepath, filename, extension,
		texture, textureUploadHeap, cpuHandle, gpuHandle } });
	textureIndex++;

	wprintf(L"Successfully loaded EXR texture: %s, location is %d\n", wideFilename.c_str(), textureIndex - 1);
}

static void CreateMipMapTextureBuffer(
	ComPtr<ID3D12Device>& device,
	ComPtr<ID3D12GraphicsCommandList>& commandList,
	const string& filepath,
	const string& lowerFilepath,
	shared_ptr<Mesh>& newMesh,
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle,
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle,
	UINT& textureIndex,
	unordered_map<string, TextureInfo>& textureInfos, const bool useSRGB)
{
	UINT size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	cpuHandle.Offset(size * textureIndex);
	gpuHandle.Offset(size * textureIndex);

	auto image = make_unique<ScratchImage>();

	wstring wideFilename = StringToWString(filepath);
	DirectX::TexMetadata metaData;
	ThrowIfFailed(DirectX::LoadFromWICFile(wideFilename.c_str(), DirectX::WIC_FLAGS_NONE, &metaData, *image));

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
	textureDesc.Format = useSRGB ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
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
	ComPtr<ID3D12Resource> textureUploadHeap;
	ThrowIfFailed(device->CreateCommittedResource(
		&uploadHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&uploadHeapdesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&textureUploadHeap)));

	UpdateSubresources(
		commandList.Get(), texture.Get(), textureUploadHeap.Get(),
		0, 0, static_cast<unsigned int>(subresources.size()), subresources.data());

	SetBarrier(commandList, texture,
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = textureDesc.Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = mipLevels;
	srvDesc.Texture2D.MostDetailedMip = 0;

	device->CreateShaderResourceView(
		texture.Get(), // 텍스처 리소스
		&srvDesc, // SRV 설명
		cpuHandle // 디스크립터 힙의 핸들
	);

	if (lowerFilepath.find("albedo") != std::string::npos || lowerFilepath.find("base") != std::string::npos)
		newMesh->textureIndexConstsBufferData.albedoIndex = textureIndex;
	else if (lowerFilepath.find("diffuse") != std::string::npos)
		newMesh->textureIndexConstsBufferData.diffuseIndex = textureIndex;
	else if (lowerFilepath.find("specular") != std::string::npos)
		newMesh->textureIndexConstsBufferData.specularIndex = textureIndex;
	else if (lowerFilepath.find("normal") != std::string::npos)
		newMesh->textureIndexConstsBufferData.normalIndex = textureIndex;
	else if (lowerFilepath.find("height") != std::string::npos)
		newMesh->textureIndexConstsBufferData.heightIndex = textureIndex;
	else if (lowerFilepath.find("ao") != std::string::npos)
		newMesh->textureIndexConstsBufferData.aoIndex = textureIndex;
	else if (lowerFilepath.find("metallic") != std::string::npos)
		newMesh->textureIndexConstsBufferData.metallicIndex = textureIndex;
	else if (lowerFilepath.find("roughness") != std::string::npos)
		newMesh->textureIndexConstsBufferData.roughnessIndex = textureIndex;
	else if (lowerFilepath.find("emissive") != std::string::npos)
		newMesh->textureIndexConstsBufferData.emissiveIndex = textureIndex;
	else
		assert(false && "MipMap texture file does not exist!");

	std::filesystem::path filePath(filepath);

	std::string filename = filePath.stem().string();  // 확장자 제외한 파일명
	std::string extension = filePath.extension().string();  // 확장자
	std::string filenameExtension = filePath.filename().string();

	textureInfos.insert({ filenameExtension, TextureInfo{
		textureIndex, filepath, filename, extension,
		texture, textureUploadHeap, cpuHandle, gpuHandle } });
	textureIndex++;

	wprintf(L"Successfully loaded MipMap texture: %s, location is %d\n", wideFilename.c_str(), textureIndex - 1);

}

static string TransformToLower(string str)
{
	std::string newStr(str.size(), '\0'); // str의 크기만큼 초기화
	// std::transform을 이용해 문자열을 소문자로 변환
	std::transform(str.begin(), str.end(), newStr.begin(), [](unsigned char c) {
		return std::tolower(c);
		});
	return newStr;
}

static void CreateTextureBuffer(
	ComPtr<ID3D12Device>& device,
	ComPtr<ID3D12GraphicsCommandList>& commandList,
	const string& filepath,
	const string& lowerFilepath,
	shared_ptr<Mesh>& newMesh,
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle,
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle,
	vector<ComPtr<ID3D12Resource>>& textures,
	vector<ComPtr<ID3D12Resource>>& texturesUploadHeap,
	UINT& textureIndex,
	unordered_map<string, TextureInfo>& textureInfos)
{
	UINT size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	cpuHandle.Offset(size * textureIndex);
	gpuHandle.Offset(size * textureIndex);

	auto image = make_unique<ScratchImage>();

	ComPtr<ID3D12Resource> texture;

	wstring wideFilename = StringToWString(filepath);
	ThrowIfFailed(DirectX::LoadFromWICFile(wideFilename.c_str(), DirectX::WIC_FLAGS_NONE, nullptr, *image));

	DirectX::TexMetadata metaData = image.get()->GetMetadata();
	ThrowIfFailed(CreateTexture(device.Get(), metaData, &texture));

	vector<D3D12_SUBRESOURCE_DATA> subresources;
	ThrowIfFailed(PrepareUpload(device.Get(), image.get()->GetImages(), image.get()->GetImageCount(), metaData, subresources));

	// upload is implemented by application developer. Here's one solution using <d3dx12.h>
	const UINT64 uploadBufferSize = GetRequiredIntermediateSize(texture.Get(), 0, static_cast<unsigned int>(subresources.size()));

	auto uploadHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto uploadHeapdesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
	ComPtr<ID3D12Resource> textureUploadHeap;
	ThrowIfFailed(device->CreateCommittedResource(
		&uploadHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&uploadHeapdesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&textureUploadHeap)));

	UpdateSubresources(
		commandList.Get(), texture.Get(), textureUploadHeap.Get(),
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
		cpuHandle // 디스크립터 힙의 핸들
	);

	if (lowerFilepath.find("albedo") != std::string::npos)
		newMesh->textureIndexConstsBufferData.albedoIndex = textureIndex;
	else if (lowerFilepath.find("diffuse") != std::string::npos)
		newMesh->textureIndexConstsBufferData.diffuseIndex = textureIndex;
	else if (lowerFilepath.find("specular") != std::string::npos)
		newMesh->textureIndexConstsBufferData.specularIndex = textureIndex;
	else if (lowerFilepath.find("normal") != std::string::npos)
		newMesh->textureIndexConstsBufferData.normalIndex = textureIndex;
	else if (lowerFilepath.find("height") != std::string::npos)
		newMesh->textureIndexConstsBufferData.heightIndex = textureIndex;
	else if (lowerFilepath.find("ao") != std::string::npos)
		newMesh->textureIndexConstsBufferData.aoIndex = textureIndex;
	else if (lowerFilepath.find("metallic") != std::string::npos)
		newMesh->textureIndexConstsBufferData.metallicIndex = textureIndex;
	else if (lowerFilepath.find("roughness") != std::string::npos)
		newMesh->textureIndexConstsBufferData.roughnessIndex = textureIndex;
	else if (lowerFilepath.find("emissive") != std::string::npos)
		newMesh->textureIndexConstsBufferData.emissiveIndex = textureIndex;
	else
		assert(false && "Texture file does not exist!");

	std::filesystem::path filePath(filepath);

	std::string filename = filePath.stem().string();  // 확장자 제외한 파일명
	std::string extension = filePath.extension().string();  // 확장자
	std::string filenameExtension = filePath.filename().string();

	textureInfos.insert({ filenameExtension, TextureInfo{
		textureIndex, filepath, filename, extension,
		texture, textureUploadHeap, cpuHandle, gpuHandle } });
	textureIndex++;

	wprintf(L"Successfully loaded texture: %s, location is %d\n", wideFilename.c_str(), textureIndex - 1);

}
