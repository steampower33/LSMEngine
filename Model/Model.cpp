#include "Model.h"

Model::Model(
	ComPtr<ID3D12Device> &device,
	ComPtr<ID3D12GraphicsCommandList> &commandList,
	CD3DX12_CPU_DESCRIPTOR_HANDLE textureHandle,
	const std::vector<MeshData>& meshDatas)
{
	Initialize(device, commandList, textureHandle, meshDatas);
}

Model::~Model()
{

}

void Model::Initialize(
	ComPtr<ID3D12Device> &device,
	ComPtr<ID3D12GraphicsCommandList> &commandList,
	CD3DX12_CPU_DESCRIPTOR_HANDLE textureHandle,
	const std::vector<MeshData>& meshDatas)
{
	CreateConstUploadBuffer(device, commandList, m_meshConstsUploadHeap, m_meshConstsBufferData, m_meshConstsBufferDataBegin);

	UINT size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	UINT textureCnt = 0;
	for (int i = 0; i < meshDatas.size(); i++)
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE newHandle = textureHandle;
		newHandle.Offset(size * textureCnt);

		std::shared_ptr<Mesh> newMesh = std::make_shared<Mesh>();

		CreateVertexBuffer(device, commandList, meshDatas[i].vertices, newMesh);
		CreateIndexBuffer(device, commandList, meshDatas[i].indices, newMesh);

		if (!meshDatas[i].diffuseFilename.empty())
		{
			CreateTextureBuffer(device, commandList, meshDatas[i].diffuseFilename, newMesh, newHandle, textures, texturesUploadHeap, texturesIdx, textureCnt);
		}
		
		CreateConstDefaultBuffer(device, commandList, newMesh);
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

	m_meshConstsBufferData.diffuseIndex = 0;

	memcpy(m_meshConstsBufferDataBegin, &m_meshConstsBufferData, sizeof(m_meshConstsBufferData));
}

void Model::Render(
	ComPtr<ID3D12Device> &device,
	ComPtr<ID3D12GraphicsCommandList> &commandList,
	ComPtr<ID3D12DescriptorHeap> &textureHeap)
{
	commandList->SetGraphicsRootConstantBufferView(1, m_meshConstsUploadHeap.Get()->GetGPUVirtualAddress());

	auto handle = textureHeap->GetGPUDescriptorHandleForHeapStart();
	auto size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	for (const auto& mesh : m_meshes)
	{
		commandList->SetGraphicsRootConstantBufferView(2, mesh->constsBuffer.Get()->GetGPUVirtualAddress());
		commandList->SetGraphicsRootDescriptorTable(3, handle);

		commandList->IASetVertexBuffers(0, 1, &mesh->vertexBufferView);
		commandList->IASetIndexBuffer(&mesh->indexBufferView);
		commandList->DrawIndexedInstanced(mesh->indexBufferCount, 1, 0, 0, 0);

	}
}