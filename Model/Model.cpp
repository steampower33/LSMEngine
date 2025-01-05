#include "Model.h"

Model::Model(
	ComPtr<ID3D12Device>& device,
	ComPtr<ID3D12GraphicsCommandList>& commandList,
	ComPtr<ID3D12CommandQueue>& commandQueue,
	CD3DX12_CPU_DESCRIPTOR_HANDLE textureHandle,
	const std::vector<MeshData>& meshDatas,
	UINT& totalTextureCnt)
{
	Initialize(device, commandList, commandQueue, textureHandle, meshDatas, totalTextureCnt);
}

Model::~Model()
{

}

void Model::Initialize(
	ComPtr<ID3D12Device>& device,
	ComPtr<ID3D12GraphicsCommandList>& commandList,
	ComPtr<ID3D12CommandQueue>& commandQueue,
	CD3DX12_CPU_DESCRIPTOR_HANDLE textureHandle,
	const std::vector<MeshData>& meshDatas,
	UINT& totalTextureCnt)
{
	CreateConstUploadBuffer(device, commandList, m_meshConstsUploadHeap, m_meshConstsBufferData, m_meshConstsBufferDataBegin);

	for (int i = 0; i < meshDatas.size(); i++)
	{
		std::shared_ptr<Mesh> newMesh = std::make_shared<Mesh>();

		CreateVertexBuffer(device, commandList, meshDatas[i].vertices, newMesh);
		CreateIndexBuffer(device, commandList, meshDatas[i].indices, newMesh);

		if (!meshDatas[i].diffuseFilename.empty())
			CreateTextureBuffer(device, commandList, meshDatas[i].diffuseFilename, newMesh, textureHandle, textures, texturesUploadHeap, texturesIdx, totalTextureCnt);

		if (!meshDatas[i].ddsFilename.empty())
			CreateDDSTextureBuffer(device, commandQueue, meshDatas[i].ddsFilename, newMesh, textureHandle, textures, texturesIdx, totalTextureCnt);

		CreateConstDefaultBuffer(device, commandList, newMesh);
		m_meshes.push_back(newMesh);
	}

	{
		XMFLOAT4 pos = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
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

		commandList->SetPipelineState(Graphics::basicPSO.Get());
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList->DrawIndexedInstanced(mesh->indexBufferCount, 1, 0, 0, 0);

		if (guiState.m_drawNormals)
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