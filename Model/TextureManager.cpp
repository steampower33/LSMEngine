#include "TextureManager.h"

TextureManager::TextureManager(
	ComPtr<ID3D12Device> device,
	HeapAllocator& srvAlloc)
{
	m_cubeTextureIndex = Graphics::textureSize;
	D3D12_DESCRIPTOR_HEAP_DESC textureHeapDesc = {};
	textureHeapDesc.NumDescriptors = Graphics::textureSize + Graphics::cubeTextureSize + Graphics::imguiTextureSize;
	textureHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	textureHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(device->CreateDescriptorHeap(&textureHeapDesc, IID_PPV_ARGS(&m_textureHeap)));

	CD3DX12_CPU_DESCRIPTOR_HANDLE heapStartCpu(m_textureHeap->GetCPUDescriptorHandleForHeapStart());
	m_heapStartCpu = heapStartCpu;

	CD3DX12_GPU_DESCRIPTOR_HANDLE heapStartGpu(m_textureHeap->GetGPUDescriptorHandleForHeapStart());
	m_heapStartGpu = heapStartGpu;

	srvAlloc.Create(device.Get(), m_textureHeap.Get());
}

void TextureManager::Initialize(
	ComPtr<ID3D12Device> device,
	ComPtr<ID3D12GraphicsCommandList> commandList)
{
	CreateEmptyTexture(device, commandList, m_heapStartCpu, m_textureIndex);
}

void TextureManager::LoadTextures(
	ComPtr<ID3D12Device> device,
	ComPtr<ID3D12GraphicsCommandList> commandList,
	ComPtr<ID3D12CommandQueue> commandQueue,
	const MeshData& meshData,
	shared_ptr<Mesh>& newMesh,
	CubemapIndexConstants& cubemapIndexConstsBufferData)
{
	CreateMipMapTexture(device, commandList, meshData.albedoFilename, newMesh, cubemapIndexConstsBufferData);
	CreateMipMapTexture(device, commandList, meshData.normalFilename, newMesh, cubemapIndexConstsBufferData);
	CreateMipMapTexture(device, commandList, meshData.heightFilename, newMesh, cubemapIndexConstsBufferData);
	CreateMipMapTexture(device, commandList, meshData.aoFilename, newMesh, cubemapIndexConstsBufferData);
	CreateMipMapTexture(device, commandList, meshData.metallicFilename, newMesh, cubemapIndexConstsBufferData);
	CreateMipMapTexture(device, commandList, meshData.roughnessFilename, newMesh, cubemapIndexConstsBufferData);
	CreateMipMapTexture(device, commandList, meshData.emissiveFilename, newMesh, cubemapIndexConstsBufferData);

	CreateDDSTexture(device, commandList, commandQueue,
		meshData.cubeEnvFilename, newMesh, cubemapIndexConstsBufferData);
	CreateDDSTexture(device, commandList, commandQueue,
		meshData.cubeDiffuseFilename, newMesh, cubemapIndexConstsBufferData);
	CreateDDSTexture(device, commandList, commandQueue,
		meshData.cubeSpecularFilename, newMesh, cubemapIndexConstsBufferData);
	CreateDDSTexture(device, commandList, commandQueue,
		meshData.cubeBrdfFilename, newMesh, cubemapIndexConstsBufferData);
}

void TextureManager::CreateMipMapTexture(
	ComPtr<ID3D12Device> device,
	ComPtr<ID3D12GraphicsCommandList> commandList,
	string filepath,
	shared_ptr<Mesh>& newMesh,
	CubemapIndexConstants& cubemapIndexConstsBufferData)
{

	string lowerFilepath = TransformToLower(filepath);

	if (CheckDuplcateFilename(m_textureInfos, filepath, lowerFilepath, newMesh, cubemapIndexConstsBufferData))
	{
		if (lowerFilepath.find(".exr") != std::string::npos)
			CreateEXRTextureBuffer(
				device, commandList,
				filepath, lowerFilepath, newMesh,
				m_heapStartCpu, m_heapStartGpu, m_textureIndex, m_textureInfos);
		else
			CreateMipMapTextureBuffer(
				device, commandList,
				filepath, lowerFilepath, newMesh,
				m_heapStartCpu, m_heapStartGpu, m_textureIndex, m_textureInfos, true);
	}
}

void TextureManager::CreateDDSTexture(
	ComPtr<ID3D12Device> device,
	ComPtr<ID3D12GraphicsCommandList> commandList,
	ComPtr<ID3D12CommandQueue> commandQueue,
	string filepath,
	shared_ptr<Mesh>& newMesh,
	CubemapIndexConstants& cubemapIndexConstsBufferData)
{
	string lowerFilepath = TransformToLower(filepath);

	if (CheckDuplcateFilename(m_ddsTextureInfos, filepath, lowerFilepath, newMesh, cubemapIndexConstsBufferData))
	{
		if (lowerFilepath.find("brdf") != std::string::npos)
			CreateDDSTextureBuffer(
				device, commandQueue,
				filepath, lowerFilepath, newMesh,
				m_heapStartCpu, m_heapStartGpu, m_textureIndex, m_ddsTextureInfos, cubemapIndexConstsBufferData, false);
		else
			CreateDDSTextureBuffer(
				device, commandQueue,
				filepath, lowerFilepath, newMesh,
				m_heapStartCpu, m_heapStartGpu, m_cubeTextureIndex, m_ddsTextureInfos, cubemapIndexConstsBufferData, true);
	}
}

bool TextureManager::CheckDuplcateFilename(
	unordered_map<string, TextureInfo>& textureIdx,
	const string& filepath,
	const string& lowerFilepath,
	shared_ptr<Mesh>& newMesh,
	CubemapIndexConstants& cubemapIndexConstsBufferData)
{
	if (filepath.empty())
		return false;

	std::filesystem::path filePath(filepath);

	std::string filenameExtension = filePath.filename().string();

	auto f = textureIdx.find(filenameExtension);

	if (f != textureIdx.end())
	{
		// Check DDS
		if (f->second.ext.find(".dds") != std::string::npos)
		{
			if (f->first.find("env") != std::string::npos)
				cubemapIndexConstsBufferData.cubemapEnvIndex = f->second.heapIndex;
			else if (f->first.find("diffuse") != std::string::npos)
				cubemapIndexConstsBufferData.cubemapDiffuseIndex = f->second.heapIndex;
			else if (f->first.find("specular") != std::string::npos)
				cubemapIndexConstsBufferData.cubemapSpecularIndex = f->second.heapIndex;
			else if (f->first.find("brdf") != std::string::npos)
				cubemapIndexConstsBufferData.brdfIndex = f->second.heapIndex;
		}
		else // Check Others
		{
			if (f->first.find("albedo") != std::string::npos || f->first.find("base") != std::string::npos)
				newMesh->textureIndexConstsBufferData.albedoIndex = f->second.heapIndex;
			else if (f->first.find("diffuse") != std::string::npos)
				newMesh->textureIndexConstsBufferData.diffuseIndex = f->second.heapIndex;
			else if (f->first.find("specular") != std::string::npos)
				newMesh->textureIndexConstsBufferData.specularIndex = f->second.heapIndex;
			else if (f->first.find("normal") != std::string::npos)
				newMesh->textureIndexConstsBufferData.normalIndex = f->second.heapIndex;
			else if (f->first.find("height") != std::string::npos)
				newMesh->textureIndexConstsBufferData.heightIndex = f->second.heapIndex;
			else if (f->first.find("ao") != std::string::npos)
				newMesh->textureIndexConstsBufferData.aoIndex = f->second.heapIndex;
			else if (f->first.find("metallic") != std::string::npos)
				newMesh->textureIndexConstsBufferData.metallicIndex = f->second.heapIndex;
			else if (f->first.find("roughness") != std::string::npos)
				newMesh->textureIndexConstsBufferData.roughnessIndex = f->second.heapIndex;
			else if (f->first.find("emissive") != std::string::npos)
				newMesh->textureIndexConstsBufferData.emissiveIndex = f->second.heapIndex;
		}

		printf("Duplicated texture : %s, location is %d\n", f->first.c_str(), f->second.heapIndex);

		return false;
	}
	else
		return true;
}


void TextureManager::CreateEmptyTexture(
	ComPtr<ID3D12Device> device,
	ComPtr<ID3D12GraphicsCommandList> commandList,
	CD3DX12_CPU_DESCRIPTOR_HANDLE textureHandle,
	UINT& textureCnt)
{
	// 1x1 검정 텍스처 데이터 (RGBA)
	unsigned char blackPixel[4] = { 0, 0, 0, 0 };

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
		IID_PPV_ARGS(&m_emptyTexture)
	));

	// 업로드 힙 리소스 생성
	auto uploadHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	const UINT64 uploadBufferSize = GetRequiredIntermediateSize(m_emptyTexture.Get(), 0, 1);
	auto buffer = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
	ThrowIfFailed(device->CreateCommittedResource(
		&uploadHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&buffer,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_emptyTextureUploadHeap)));

	// 텍스처 데이터 복사
	D3D12_SUBRESOURCE_DATA textureData = {};
	textureData.pData = blackPixel;
	textureData.RowPitch = 4; // 1x1 픽셀, RGBA
	textureData.SlicePitch = textureData.RowPitch;

	UpdateSubresources(commandList.Get(), m_emptyTexture.Get(), m_emptyTextureUploadHeap.Get(), 0, 0, 1, &textureData);

	SetBarrier(commandList, m_emptyTexture,
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = m_emptyTexture->GetDesc().Format; // 텍스처의 포맷
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	UINT size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	device->CreateShaderResourceView(
		m_emptyTexture.Get(), // 텍스처 리소스
		&srvDesc, // SRV 설명
		textureHandle // 디스크립터 힙의 핸들
	);

	textureCnt++;
}