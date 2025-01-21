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
	if (CheckDuplcateFilename(m_textureIdx, filename, newMesh, cubemapIndexConstsBufferData))
	{
		if (filename.find(".exr") != std::string::npos)
			CreateEXRTextureBuffer(
				device, commandList,
				filename, newMesh,
				m_heapStartCpu, m_textures, m_texturesUploadHeap,
				m_textureCnt, m_textureIdx);
		else
			CreateMipMapTextureBuffer(
				device, commandList,
				filename, newMesh,
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
	if (CheckDuplcateFilename(m_textureIdx, filename, newMesh, cubemapIndexConstsBufferData))
	{
		if (filename.find("Brdf") != std::string::npos)
			CreateDDSTextureBuffer(
				device, commandQueue,
				filename, newMesh,
				m_heapStartCpu, m_textures,
				m_textureCnt, m_textureIdx, cubemapIndexConstsBufferData, false);
		else
			CreateDDSTextureBuffer(
				device, commandQueue,
				filename, newMesh,
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
			if (filename.find("Env") != std::string::npos)
				cubemapIndexConstsBufferData.cubemapEnvIndex = f->second;
			else if (filename.find("Diffuse") != std::string::npos)
				cubemapIndexConstsBufferData.cubemapDiffuseIndex = f->second;
			else if (filename.find("Specular") != std::string::npos)
				cubemapIndexConstsBufferData.cubemapSpecularIndex = f->second;
			else if (filename.find("Brdf") != std::string::npos)
				newMesh->constsBufferData.brdfIndex = f->second;
		}
		else // Check Others
		{
			if (filename.find("albedo") != std::string::npos)
				newMesh->constsBufferData.albedoIndex = f->second;
			else if (filename.find("diffuse") != std::string::npos)
				newMesh->constsBufferData.diffuseIndex = f->second;
			else if (filename.find("specular") != std::string::npos)
				newMesh->constsBufferData.specularIndex = f->second;
			else if (filename.find("normal") != std::string::npos)
				newMesh->constsBufferData.normalIndex = f->second;
			else if (filename.find("height") != std::string::npos)
				newMesh->constsBufferData.heightIndex = f->second;
			else if (filename.find("ao") != std::string::npos)
				newMesh->constsBufferData.aoIndex = f->second;
			else if (filename.find("metallic") != std::string::npos)
				newMesh->constsBufferData.metallicIndex = f->second;
			else if (filename.find("roughness") != std::string::npos)
				newMesh->constsBufferData.roughnessIndex = f->second;
		}

		printf("Duplicated texture : %s, location is %d\n", f->first.c_str(), f->second);

		return false;
	}
	else
		return true;
}

