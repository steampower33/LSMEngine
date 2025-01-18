#include "TextureManager.h"

TextureManager::TextureManager(
	ComPtr<ID3D12Device>& device,
	ComPtr<ID3D12GraphicsCommandList>& commandList, 
	ComPtr<ID3D12DescriptorHeap>& textureHeap)
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE heapStartCpu(textureHeap->GetCPUDescriptorHandleForHeapStart());

	m_heapStartCpu = heapStartCpu;

	CreateEmptyTexture(device, commandList, heapStartCpu, m_textures, m_texturesUploadHeap, m_normaltextureCnt);
}

void TextureManager::LoadTextures(
	ComPtr<ID3D12Device>& device,
	ComPtr<ID3D12GraphicsCommandList>& commandList,
	ComPtr<ID3D12CommandQueue>& commandQueue,
	const MeshData& meshData,
	shared_ptr<Mesh>& newMesh,
	CubemapIndexConstants& cubemapIndexConstsBufferData)
{
	// ETC
	if (CheckDuplcateFilename(m_textureIdx, meshData.colorFilename, newMesh, cubemapIndexConstsBufferData))
		CreateMipMapTextureBuffer(
			device, commandList, 
			meshData.colorFilename, newMesh, 
			m_heapStartCpu, m_textures, m_texturesUploadHeap, 
			m_normaltextureCnt, m_textureIdx);

	// DDS
	if (CheckDuplcateFilename(m_textureIdx, meshData.ddsColorFilename, newMesh, cubemapIndexConstsBufferData))
		CreateDDSTextureBuffer(
			device, commandQueue, 
			meshData.ddsColorFilename, newMesh, 
			m_heapStartCpu, m_textures, 
			m_cubeTextureCnt, m_textureIdx, cubemapIndexConstsBufferData);

	if (CheckDuplcateFilename(m_textureIdx, meshData.ddsDiffuseFilename, newMesh, cubemapIndexConstsBufferData))
		CreateDDSTextureBuffer(
			device, commandQueue, 
			meshData.ddsDiffuseFilename, 
			newMesh, m_heapStartCpu, m_textures, 
			m_cubeTextureCnt, m_textureIdx, cubemapIndexConstsBufferData);

	if (CheckDuplcateFilename(m_textureIdx, meshData.ddsSpecularFilename, newMesh, cubemapIndexConstsBufferData))
		CreateDDSTextureBuffer(
			device, commandQueue, 
			meshData.ddsSpecularFilename, 
			newMesh, m_heapStartCpu, m_textures, 
			m_cubeTextureCnt, m_textureIdx, cubemapIndexConstsBufferData);
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
			if (filename.find("Color") != std::string::npos)
				cubemapIndexConstsBufferData.cubemapColorIndex = f->second;
			else if (filename.find("Diffuse") != std::string::npos)
				cubemapIndexConstsBufferData.cubemapDiffuseIndex = f->second;
			else if (filename.find("Specular") != std::string::npos)
				cubemapIndexConstsBufferData.cubemapSpecularIndex = f->second;
		}
		else // Check Others
		{
			if (filename.find("Color") != std::string::npos)
				newMesh->constsBufferData.colorIndex = f->second;
			else if (filename.find("Diffuse") != std::string::npos)
				newMesh->constsBufferData.diffuseIndex = f->second;
			else if (filename.find("Specular") != std::string::npos)
				newMesh->constsBufferData.specularIndex = f->second;
		}

		printf("Duplicated texture : %s, location is %d\n", f->first.c_str(), f->second);

		return false;
	}
	else
		return true;
}

