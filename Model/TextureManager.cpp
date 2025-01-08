#include "TextureManager.h"

void TextureManager::LoadTextures(
	ComPtr<ID3D12Device>& device,
	ComPtr<ID3D12GraphicsCommandList>& commandList,
	ComPtr<ID3D12CommandQueue>& commandQueue,
	const MeshData& meshData,
	shared_ptr<Mesh>& newMesh,
	CubemapIndexConstants& cubemapIndexConstsBufferData)
{
	// ETC
	if (CheckDuplcateFilename(m_textureIdx, meshData.diffuseFilename, newMesh))
		CreateTextureBuffer(
			device, commandList, 
			meshData.diffuseFilename, newMesh, 
			m_heapStartCpu, m_textures, m_texturesUploadHeap, 
			m_normaltextureCnt, m_textureIdx);

	// DDS
	if (CheckDuplcateFilename(m_textureIdx, meshData.ddsAmbientFilename, newMesh))
		CreateDDSTextureBuffer(
			device, commandQueue, 
			meshData.ddsAmbientFilename, newMesh, 
			m_heapStartCpu, m_textures, 
			m_cubeTextureCnt, m_textureIdx, cubemapIndexConstsBufferData);

	if (CheckDuplcateFilename(m_textureIdx, meshData.ddsDiffuseFilename, newMesh))
		CreateDDSTextureBuffer(
			device, commandQueue, 
			meshData.ddsDiffuseFilename, 
			newMesh, m_heapStartCpu, m_textures, 
			m_cubeTextureCnt, m_textureIdx, cubemapIndexConstsBufferData);

	if (CheckDuplcateFilename(m_textureIdx, meshData.ddsSpecularFilename, newMesh))
		CreateDDSTextureBuffer(
			device, commandQueue, 
			meshData.ddsSpecularFilename, 
			newMesh, m_heapStartCpu, m_textures, 
			m_cubeTextureCnt, m_textureIdx, cubemapIndexConstsBufferData);
}

void TextureManager::SetTextureHandle(ComPtr<ID3D12DescriptorHeap>& textureHeap)
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE heapStartCpu(textureHeap->GetCPUDescriptorHandleForHeapStart());

	m_heapStartCpu = heapStartCpu;
}

bool TextureManager::CheckDuplcateFilename(
	unordered_map<string, int>& textureIdx,
	const string& filename,
	shared_ptr<Mesh>& newMesh)
{
	if (filename.empty())
		return false;

	auto f = textureIdx.find(filename);
	if (f != textureIdx.end())
	{
		newMesh->constsBufferData.diffuseIndex = f->second;
		return false;
	}
	else
		return true;
}