#include "Model.h"

Model::Model(
	ComPtr<ID3D12Device>& device,
	ComPtr<ID3D12GraphicsCommandList>& commandList,
	ComPtr<ID3D12CommandQueue>& commandQueue,
	CD3DX12_CPU_DESCRIPTOR_HANDLE textureHandle,
	const vector<MeshData>& meshDatas,
	UINT& totalTextureCnt,
	unordered_map<string, int>& textureIdx)
{
	Initialize(device, commandList, commandQueue, textureHandle, meshDatas, totalTextureCnt, textureIdx);
}

Model::~Model()
{

}

bool Model::CheckDuplcateFilename(
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

void Model::Initialize(
	ComPtr<ID3D12Device>& device,
	ComPtr<ID3D12GraphicsCommandList>& commandList,
	ComPtr<ID3D12CommandQueue>& commandQueue,
	CD3DX12_CPU_DESCRIPTOR_HANDLE textureHandle,
	const vector<MeshData>& meshDatas,
	UINT& totalTextureCnt,
	unordered_map<string, int>& textureIdx)
{
	CreateConstUploadBuffer(device, commandList, m_meshConstsUploadHeap, m_meshConstsBufferData, m_meshConstsBufferDataBegin);

	for (int i = 0; i < meshDatas.size(); i++)
	{
		shared_ptr<Mesh> newMesh = make_shared<Mesh>();

		CreateVertexBuffer(device, commandList, meshDatas[i].vertices, newMesh);
		CreateIndexBuffer(device, commandList, meshDatas[i].indices, newMesh);
		
		if (CheckDuplcateFilename(textureIdx, meshDatas[i].diffuseFilename, newMesh))
			CreateTextureBuffer(device, commandList, meshDatas[i].diffuseFilename, newMesh, textureHandle, textures, texturesUploadHeap, totalTextureCnt, textureIdx);

		if (CheckDuplcateFilename(textureIdx, meshDatas[i].ddsFilename, newMesh))
			CreateDDSTextureBuffer(device, commandQueue, meshDatas[i].ddsFilename, newMesh, textureHandle, textures, totalTextureCnt, textureIdx);

		CreateConstDefaultBuffer(device, commandList, newMesh);
		m_meshes.push_back(newMesh);
	}

	{
		XMVECTOR posVec = XMLoadFloat4(&pos);

		XMMATRIX world = XMMatrixTranspose(XMMatrixTranslationFromVector(posVec));

		XMStoreFloat4x4(&m_meshConstsBufferData.world, world);

		world.r[3] = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);

		// 역행렬 계산
		XMMATRIX worldInv = XMMatrixInverse(nullptr, world);

		// 역행렬의 전치 계산
		XMMATRIX worldInvTranspose = XMMatrixTranspose(worldInv);

		XMStoreFloat4x4(&m_meshConstsBufferData.worldIT, worldInvTranspose);
	}
}

void Model::Update()
{
	XMVECTOR posVec = XMLoadFloat4(&pos);

	XMMATRIX world = XMMatrixTranspose(XMMatrixTranslationFromVector(posVec));

	XMStoreFloat4x4(&m_meshConstsBufferData.world, world);

	world.r[3] = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);

	// 역행렬 계산
	XMMATRIX worldInv = XMMatrixInverse(nullptr, world);

	// 역행렬의 전치 계산
	XMMATRIX worldInvTranspose = XMMatrixTranspose(worldInv);

	XMStoreFloat4x4(&m_meshConstsBufferData.worldIT, worldInvTranspose);

	memcpy(m_meshConstsBufferDataBegin, &m_meshConstsBufferData, sizeof(m_meshConstsBufferData));
}

void Model::Render(
	ComPtr<ID3D12Device>& device,
	ComPtr<ID3D12GraphicsCommandList>& commandList,
	ComPtr<ID3D12DescriptorHeap>& textureHeap,
	GuiState& guiState)
{
	commandList->SetGraphicsRootConstantBufferView(1, m_meshConstsUploadHeap.Get()->GetGPUVirtualAddress());

	auto textureHandle = textureHeap->GetGPUDescriptorHandleForHeapStart();
	auto size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	for (const auto& mesh : m_meshes)
	{
		commandList->SetGraphicsRootConstantBufferView(2, mesh->constsBuffer.Get()->GetGPUVirtualAddress());
		commandList->SetGraphicsRootDescriptorTable(3, textureHandle);

		commandList->IASetVertexBuffers(0, 1, &mesh->vertexBufferView);
		commandList->IASetIndexBuffer(&mesh->indexBufferView);

		if (guiState.isWireframe)
			commandList->SetPipelineState(Graphics::basicWirePSO.Get());
		else
			commandList->SetPipelineState(Graphics::basicSolidPSO.Get());

		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList->DrawIndexedInstanced(mesh->indexBufferCount, 1, 0, 0, 0);

		if (guiState.isDrawNormals)
		{
			commandList->SetPipelineState(Graphics::normalPSO.Get());
			commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
			commandList->DrawInstanced(mesh->vertexBufferCount, 1, 0, 0);
		}
	}
}


void Model::RenderSkybox(
	ComPtr<ID3D12Device>& device,
	ComPtr<ID3D12GraphicsCommandList>& commandList,
	ComPtr<ID3D12DescriptorHeap>& textureHeap,
	GuiState& guiState)
{
	commandList->SetGraphicsRootConstantBufferView(1, m_meshConstsUploadHeap.Get()->GetGPUVirtualAddress());
	commandList->SetGraphicsRootConstantBufferView(2, m_meshes[0]->constsBuffer.Get()->GetGPUVirtualAddress());
	commandList->SetGraphicsRootDescriptorTable(3, textureHeap->GetGPUDescriptorHandleForHeapStart());

	commandList->IASetVertexBuffers(0, 1, &m_meshes[0]->vertexBufferView);
	commandList->IASetIndexBuffer(&m_meshes[0]->indexBufferView);

	commandList->SetPipelineState(Graphics::skyboxPSO.Get());
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->DrawIndexedInstanced(m_meshes[0]->indexBufferCount, 1, 0, 0, 0);
}