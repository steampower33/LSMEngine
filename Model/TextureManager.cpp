#include "TextureManager.h"

TextureManager::TextureManager(
	ComPtr<ID3D12Device>& device,
	ComPtr<ID3D12GraphicsCommandList>& commandList,
	ComPtr<ID3D12DescriptorHeap>& textureHeap)
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE heapStartCpu(textureHeap->GetCPUDescriptorHandleForHeapStart());

	m_heapStartCpu = heapStartCpu;

	CreateEmptyTexture(device, commandList, heapStartCpu, m_textures, m_texturesUploadHeap, m_textureCnt);
}

void TextureManager::CreateMipMapTexture(
	ComPtr<ID3D12Device>& device,
	ComPtr<ID3D12GraphicsCommandList>& commandList,
	string filename,
	shared_ptr<Mesh>& newMesh,
	CubemapIndexConstants& cubemapIndexConstsBufferData)
{
	string lowerFilename = TransformToLower(filename);
	if (CheckDuplcateFilename(m_textureIdx, filename, lowerFilename, newMesh, cubemapIndexConstsBufferData))
	{
		if (filename.find(".exr") != std::string::npos)
			CreateEXRTextureBuffer(
				device, commandList,
				filename, lowerFilename, newMesh,
				m_heapStartCpu, m_textures, m_texturesUploadHeap,
				m_textureCnt, m_textureIdx);
		else
			CreateMipMapTextureBuffer(
				device, commandList,
				filename, lowerFilename, newMesh,
				m_heapStartCpu, m_textures, m_texturesUploadHeap,
				m_textureCnt, m_textureIdx, true);
	}
}

void TextureManager::CreateDDSTexture(
	ComPtr<ID3D12Device>& device,
	ComPtr<ID3D12GraphicsCommandList>& commandList,
	ComPtr<ID3D12CommandQueue>& commandQueue,
	string filename,
	shared_ptr<Mesh>& newMesh,
	CubemapIndexConstants& cubemapIndexConstsBufferData)
{
	string lowerFilename = TransformToLower(filename);

	if (CheckDuplcateFilename(m_textureIdx, filename, lowerFilename, newMesh, cubemapIndexConstsBufferData))
	{
		if (lowerFilename.find("brdf") != std::string::npos)
			CreateDDSTextureBuffer(
				device, commandQueue,
				filename, lowerFilename, newMesh,
				m_heapStartCpu, m_textures,
				m_textureCnt, m_textureIdx, cubemapIndexConstsBufferData, false);
		else
			CreateDDSTextureBuffer(
				device, commandQueue,
				filename, lowerFilename, newMesh,
				m_heapStartCpu, m_textures,
				m_cubeTextureCnt, m_textureIdx, cubemapIndexConstsBufferData, true);
	}
}

void TextureManager::LoadTextures(
	ComPtr<ID3D12Device>& device,
	ComPtr<ID3D12GraphicsCommandList>& commandList,
	ComPtr<ID3D12CommandQueue>& commandQueue,
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

bool TextureManager::CheckDuplcateFilename(
	unordered_map<string, int>& textureIdx,
	const string& filename,
	const string& lowerFilename,
	shared_ptr<Mesh>& newMesh,
	CubemapIndexConstants& cubemapIndexConstsBufferData)
{
	if (filename.empty())
		return false;

	auto f = textureIdx.find(filename);
	if (f != textureIdx.end())
	{
		// Check DDS
		if (f->first.find(".dds") != std::string::npos)
		{
			if (lowerFilename.find("env") != std::string::npos)
				cubemapIndexConstsBufferData.cubemapEnvIndex = f->second;
			else if (lowerFilename.find("diffuse") != std::string::npos)
				cubemapIndexConstsBufferData.cubemapDiffuseIndex = f->second;
			else if (lowerFilename.find("specular") != std::string::npos)
				cubemapIndexConstsBufferData.cubemapSpecularIndex = f->second;
			else if (lowerFilename.find("brdf") != std::string::npos)
				cubemapIndexConstsBufferData.brdfIndex = f->second;
		}
		else // Check Others
		{
			if (lowerFilename.find("albedo") != std::string::npos)
				newMesh->constsBufferData.albedoIndex = f->second;
			else if (lowerFilename.find("diffuse") != std::string::npos)
				newMesh->constsBufferData.diffuseIndex = f->second;
			else if (lowerFilename.find("specular") != std::string::npos)
				newMesh->constsBufferData.specularIndex = f->second;
			else if (lowerFilename.find("normal") != std::string::npos)
				newMesh->constsBufferData.normalIndex = f->second;
			else if (lowerFilename.find("height") != std::string::npos)
				newMesh->constsBufferData.heightIndex = f->second;
			else if (lowerFilename.find("ao") != std::string::npos)
				newMesh->constsBufferData.aoIndex = f->second;
			else if (lowerFilename.find("metallic") != std::string::npos)
				newMesh->constsBufferData.metallicIndex = f->second;
			else if (lowerFilename.find("roughness") != std::string::npos)
				newMesh->constsBufferData.roughnessIndex = f->second;
			else if (lowerFilename.find("emissive") != std::string::npos)
				newMesh->constsBufferData.emissiveIndex = f->second;
		}

		printf("Duplicated texture : %s, location is %d\n", f->first.c_str(), f->second);

		return false;
	}
	else
		return true;
}

