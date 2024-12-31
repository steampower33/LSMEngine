#include "Model.h"

Model::Model(
	ComPtr<ID3D12Device> &device,
	ComPtr<ID3D12GraphicsCommandList> &commandList,
	CD3DX12_CPU_DESCRIPTOR_HANDLE basicHandle,
	const std::vector<MeshData>& meshData)
{
	Initialize(device, commandList, basicHandle, meshData);
}

Model::~Model()
{

}

void Model::Initialize(
	ComPtr<ID3D12Device> &device,
	ComPtr<ID3D12GraphicsCommandList> &commandList,
	CD3DX12_CPU_DESCRIPTOR_HANDLE basicHandle,
	const std::vector<MeshData>& mesheDatas)
{
	CreateConstBuffer(device, commandList, m_meshConstsUploadHeap, m_meshConstsBufferData, m_meshConstsBufferDataBegin, basicHandle);
	basicHandle.Offset(1, device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
	
	for (const auto &meshData : mesheDatas)
	{
		std::shared_ptr<Mesh> newMesh = std::make_shared<Mesh>();

		CreateVertexBuffer(device, commandList, meshData.vertices, newMesh);
		CreateIndexBuffer(device, commandList, meshData.indices, newMesh);

		CreateTextureBuffer(device, commandList, m_texture, m_textureUploadHeap, basicHandle, meshData.textureFilename);
		
		m_meshes.push_back(newMesh);
	}

}

void Model::Update()
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

	memcpy(m_meshConstsBufferDataBegin, &m_meshConstsBufferData, sizeof(m_meshConstsBufferData));
}

void Model::Render(
	ComPtr<ID3D12Device> &device,
	ComPtr<ID3D12GraphicsCommandList> &commandList)
{

	for (const auto& mesh : m_meshes)
	{
		commandList->IASetVertexBuffers(0, 1, &mesh->vertexBufferView);
		commandList->IASetIndexBuffer(&mesh->indexBufferView);
		commandList->DrawIndexedInstanced(mesh->indexBufferCount, 1, 0, 0, 0);
	}
}