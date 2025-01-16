#include "Model.h"

Model::Model(
	ComPtr<ID3D12Device>& device,
	ComPtr<ID3D12GraphicsCommandList>& commandList,
	ComPtr<ID3D12CommandQueue>& commandQueue,
	const vector<MeshData>& meshDatas,
	CubemapIndexConstants& cubemapIndexConstsBufferData,
	shared_ptr<TextureManager>& textureManager)
{
	Initialize(device, commandList, commandQueue, meshDatas, cubemapIndexConstsBufferData, textureManager);
}

Model::~Model()
{

}

void Model::Initialize(
	ComPtr<ID3D12Device>& device,
	ComPtr<ID3D12GraphicsCommandList>& commandList,
	ComPtr<ID3D12CommandQueue>& commandQueue,
	const vector<MeshData>& meshDatas,
	CubemapIndexConstants& cubemapIndexConstsBufferData,
	shared_ptr<TextureManager>& textureManager)
{
	// 초기 world, invTrans 설정
	XMVECTOR positionVector = XMLoadFloat4(&position);
	XMMATRIX positionMatrix = XMMatrixTranslationFromVector(positionVector);
	XMStoreFloat4x4(&world, positionMatrix);
	XMStoreFloat4x4(&m_meshConstsBufferData.world, XMMatrixTranspose(positionMatrix));
	XMStoreFloat4x4(&m_meshConstsBufferData.worldIT, XMMatrixTranspose(XMMatrixInverse(nullptr, positionMatrix)));

	CreateConstUploadBuffer(device, commandList, m_meshConstsUploadHeap, m_meshConstsBufferData, m_meshConstsBufferDataBegin);

	for (int i = 0; i < meshDatas.size(); i++)
	{
		shared_ptr<Mesh> newMesh = make_shared<Mesh>();

		CreateVertexBuffer(device, commandList, meshDatas[i].vertices, newMesh);
		CreateIndexBuffer(device, commandList, meshDatas[i].indices, newMesh);

		textureManager->LoadTextures(device, commandList, commandQueue, meshDatas[i], newMesh, cubemapIndexConstsBufferData);

		CreateConstDefaultBuffer(device, commandList, newMesh);
		m_meshes.push_back(newMesh);
	}
}

void Model::Update(XMVECTOR& q)
{
	XMMATRIX worldMatrix = XMLoadFloat4x4(&world);
	XMVECTOR translation = worldMatrix.r[3];

	XMMATRIX worldWithoutTranslation = worldMatrix;
	worldWithoutTranslation.r[3] = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);

	XMMATRIX rotationMatrix = XMMatrixRotationQuaternion(q);

	XMMATRIX translationMatrix = XMMatrixTranslationFromVector(translation);

	XMMATRIX newWorld = worldWithoutTranslation * rotationMatrix * translationMatrix;

	XMStoreFloat4x4(&world, newWorld);

	XMStoreFloat4x4(&m_meshConstsBufferData.world, XMMatrixTranspose(newWorld));

	newWorld.r[3] = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
	XMMATRIX worldInvTranspose = XMMatrixTranspose(XMMatrixInverse(nullptr, newWorld));
	XMStoreFloat4x4(&m_meshConstsBufferData.worldIT, worldInvTranspose);

	OnlyCallConstsMemcpy();
}

void Model::OnlyCallConstsMemcpy()
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
	commandList->SetGraphicsRootDescriptorTable(4, textureHeap->GetGPUDescriptorHandleForHeapStart());

	if (guiState.isWireframe)
		commandList->SetPipelineState(Graphics::basicWirePSO.Get());
	else
		commandList->SetPipelineState(Graphics::basicSolidPSO.Get());

	for (const auto& mesh : m_meshes)
	{
		commandList->SetGraphicsRootConstantBufferView(2, mesh->constsBuffer.Get()->GetGPUVirtualAddress());

		commandList->IASetVertexBuffers(0, 1, &mesh->vertexBufferView);
		commandList->IASetIndexBuffer(&mesh->indexBufferView);

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
	commandList->SetGraphicsRootDescriptorTable(4, textureHeap->GetGPUDescriptorHandleForHeapStart());

	commandList->IASetVertexBuffers(0, 1, &m_meshes[0]->vertexBufferView);
	commandList->IASetIndexBuffer(&m_meshes[0]->indexBufferView);

	commandList->SetPipelineState(Graphics::skyboxPSO.Get());
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->DrawIndexedInstanced(m_meshes[0]->indexBufferCount, 1, 0, 0, 0);
}