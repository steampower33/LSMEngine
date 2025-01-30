#include "Model.h"

Model::Model(
	ComPtr<ID3D12Device>& device,
	ComPtr<ID3D12GraphicsCommandList>& commandList,
	ComPtr<ID3D12CommandQueue>& commandQueue,
	const vector<MeshData>& meshDatas,
	CubemapIndexConstants& cubemapIndexConstsBufferData,
	shared_ptr<TextureManager>& textureManager,
	XMFLOAT4 position)
{
	m_position = position;
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
	m_radius = 1.0f;
	m_boundingSphere = make_shared<BoundingSphere>(XMFLOAT3(m_position.x, m_position.y, m_position.z), m_radius);

	// 초기 world, invTrans 설정
	XMVECTOR positionVector = XMLoadFloat4(&m_position);
	XMMATRIX positionMatrix = XMMatrixTranslationFromVector(positionVector);
	XMStoreFloat4x4(&m_world, positionMatrix);
	XMStoreFloat4x4(&m_meshConstsBufferData.world, XMMatrixTranspose(positionMatrix));
	XMStoreFloat4x4(&m_meshConstsBufferData.worldIT, XMMatrixInverse(nullptr, positionMatrix));

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

void Model::Update()
{
	XMMATRIX scaleMatrix = XMMatrixScaling(m_scale, m_scale, m_scale);

	XMVECTOR translation = XMLoadFloat4(&m_position);
	XMMATRIX translationMatrix = XMMatrixTranslationFromVector(translation);

	XMMATRIX newWorld = scaleMatrix * translationMatrix;
	XMStoreFloat3(&m_boundingSphere->Center, newWorld.r[3]);
	XMStoreFloat4x4(&m_world, newWorld);

	XMStoreFloat4x4(&m_meshConstsBufferData.world, XMMatrixTranspose(newWorld));

	newWorld.r[3] = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
	XMMATRIX worldInvTranspose = XMMatrixInverse(nullptr, newWorld);
	XMStoreFloat4x4(&m_meshConstsBufferData.worldIT, worldInvTranspose);

	OnlyCallConstsMemcpy();
}

void Model::UpdateQuaternionAndTranslation(XMVECTOR& q, XMVECTOR& dragTranslation)
{
	XMMATRIX worldMatrix = XMLoadFloat4x4(&m_world);
	XMVECTOR translation = worldMatrix.r[3];

	XMMATRIX worldWithoutTranslation = worldMatrix;
	worldWithoutTranslation.r[3] = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);

	XMMATRIX rotationMatrix = XMMatrixRotationQuaternion(q);

	XMMATRIX translationMatrix = XMMatrixTranslationFromVector(translation);
	XMMATRIX dragTranslationMatrix = XMMatrixTranslationFromVector(dragTranslation);

	XMMATRIX newWorld = worldWithoutTranslation * rotationMatrix * translationMatrix * dragTranslationMatrix;

	XMStoreFloat3(&m_boundingSphere->Center, newWorld.r[3]);

	XMStoreFloat4x4(&m_world, newWorld);

	XMStoreFloat4x4(&m_meshConstsBufferData.world, XMMatrixTranspose(newWorld));

	newWorld.r[3] = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
	XMMATRIX worldInvTranspose = XMMatrixInverse(nullptr, newWorld);
	XMStoreFloat4x4(&m_meshConstsBufferData.worldIT, worldInvTranspose);

	OnlyCallConstsMemcpy();
}

void Model::UpdateState()
{
	m_meshConstsBufferData.useAlbedoMap = m_useAlbedoMap;
	m_meshConstsBufferData.useNormalMap = m_useNormalMap;
	m_meshConstsBufferData.useHeightMap = m_useHeightMap;
	m_meshConstsBufferData.useAOMap = m_useAOMap;
	m_meshConstsBufferData.useMetallicMap = m_useMetallicMap;
	m_meshConstsBufferData.useRoughnessMap = m_useRoughnessMap;
	m_meshConstsBufferData.useEmissiveMap = m_useEmissiveMap;
	memcpy(m_meshConstsBufferDataBegin, &m_meshConstsBufferData, sizeof(m_meshConstsBufferData));
}

void Model::OnlyCallConstsMemcpy()
{
	memcpy(m_meshConstsBufferDataBegin, &m_meshConstsBufferData, sizeof(m_meshConstsBufferData));
}

void Model::Render(
	ComPtr<ID3D12Device>& device,
	ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	commandList->SetGraphicsRootConstantBufferView(1, m_meshConstsUploadHeap.Get()->GetGPUVirtualAddress());

	for (const auto& mesh : m_meshes)
	{
		commandList->SetGraphicsRootConstantBufferView(2, mesh->textureIndexConstsBuffer.Get()->GetGPUVirtualAddress());

		commandList->IASetVertexBuffers(0, 1, &mesh->vertexBufferView);
		commandList->IASetIndexBuffer(&mesh->indexBufferView);

		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList->DrawIndexedInstanced(mesh->indexBufferCount, 1, 0, 0, 0);
	}
}

void Model::RenderSkybox(
	ComPtr<ID3D12Device>& device,
	ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	commandList->SetGraphicsRootConstantBufferView(1, m_meshConstsUploadHeap.Get()->GetGPUVirtualAddress());
	commandList->SetGraphicsRootConstantBufferView(2, m_meshes[0]->textureIndexConstsBuffer.Get()->GetGPUVirtualAddress());

	commandList->IASetVertexBuffers(0, 1, &m_meshes[0]->vertexBufferView);
	commandList->IASetIndexBuffer(&m_meshes[0]->indexBufferView);

	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->DrawIndexedInstanced(m_meshes[0]->indexBufferCount, 1, 0, 0, 0);
}

void Model::RenderNormal(
	ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	commandList->SetGraphicsRootConstantBufferView(1, m_meshConstsUploadHeap.Get()->GetGPUVirtualAddress());

	commandList->SetPipelineState(Graphics::normalPSO.Get());
	for (const auto& mesh : m_meshes)
	{
		commandList->IASetVertexBuffers(0, 1, &mesh->vertexBufferView);
		commandList->IASetIndexBuffer(&mesh->indexBufferView);

		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
		commandList->DrawInstanced(mesh->vertexBufferCount, 1, 0, 0);
	}
}