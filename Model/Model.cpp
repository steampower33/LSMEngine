#include "Model.h"

Model::Model(
	ComPtr<ID3D12Device> device,
	ComPtr<ID3D12GraphicsCommandList> commandList,
	CD3DX12_CPU_DESCRIPTOR_HANDLE basicHandle,
	const std::vector<MeshData>& meshData)
{
	Initialize(device, commandList, basicHandle, meshData);
}

Model::~Model()
{

}

void Model::Initialize(
	ComPtr<ID3D12Device> device,
	ComPtr<ID3D12GraphicsCommandList> commandList,
	CD3DX12_CPU_DESCRIPTOR_HANDLE basicHandle,
	const std::vector<MeshData>& mesheDatas)
{
	
	for (const auto &meshData : mesheDatas)
	{
		std::shared_ptr<Mesh> newMesh = std::make_shared<Mesh>();

		CreateVertexBuffer(device, commandList, meshData.vertices, newMesh);
		CreateIndexBuffer(device, commandList, meshData.indices, newMesh);

		m_meshes.push_back(newMesh);
	}


	{
		const UINT meshConstantsSize = sizeof(GlobalConstants);

		auto uploadHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto buffer = CD3DX12_RESOURCE_DESC::Buffer(meshConstantsSize);
		ThrowIfFailed(device->CreateCommittedResource(
			&uploadHeapProps,
			D3D12_HEAP_FLAG_NONE,
			&buffer,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_meshConstsUploadHeap)));

		CD3DX12_RANGE readRange(0, 0);
		ThrowIfFailed(m_meshConstsUploadHeap->Map(0, &readRange, reinterpret_cast<void**>(&m_meshConstsBufferDataBegin)));

		memcpy(m_meshConstsBufferDataBegin, &m_meshConstsBufferData, sizeof(m_meshConstsBufferData));

		// Describe and create a constant buffer view.
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = m_meshConstsUploadHeap->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = (meshConstantsSize + 255) & ~255; // 256-byte ����
		device->CreateConstantBufferView(&cbvDesc, basicHandle);

		basicHandle.Offset(1, device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
	}

	// Create the texture.
	auto image = std::make_unique<ScratchImage>();
	{
		ThrowIfFailed(DirectX::LoadFromWICFile(L"./Assets/wall.jpg", DirectX::WIC_FLAGS_NONE, nullptr, *image));

		DirectX::TexMetadata metaData = image.get()->GetMetadata();
		ThrowIfFailed(CreateTexture(device.Get(), metaData, &m_texture));

		std::vector<D3D12_SUBRESOURCE_DATA> subresources;
		ThrowIfFailed(PrepareUpload(device.Get(), image.get()->GetImages(), image.get()->GetImageCount(), metaData, subresources));

		// upload is implemented by application developer. Here's one solution using <d3dx12.h>
		const UINT64 uploadBufferSize = GetRequiredIntermediateSize(m_texture.Get(), 0, static_cast<unsigned int>(subresources.size()));

		auto uploadHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto buffer = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
		ThrowIfFailed(device->CreateCommittedResource(
			&uploadHeapProps,
			D3D12_HEAP_FLAG_NONE,
			&buffer,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_textureUploadHeap)));

		UpdateSubresources(
			commandList.Get(), m_texture.Get(), m_textureUploadHeap.Get(),
			0, 0, static_cast<unsigned int>(subresources.size()), subresources.data());

		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			m_texture.Get(), // �ؽ�ó ���ҽ�
			D3D12_RESOURCE_STATE_COPY_DEST, // ���� ����
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE // ���ο� ����
		);
		commandList->ResourceBarrier(1, &barrier);

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = metaData.format; // �ؽ�ó�� ����
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = static_cast<UINT>(metaData.mipLevels);

		device->CreateShaderResourceView(
			m_texture.Get(), // �ؽ�ó ���ҽ�
			&srvDesc, // SRV ����
			basicHandle // ��ũ���� ���� �ڵ�
		);
	}
}

void Model::Update()
{
	XMFLOAT4 pos = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	XMVECTOR posVec = XMLoadFloat4(&pos);

	XMMATRIX world = XMMatrixTranspose(XMMatrixTranslationFromVector(posVec));

	XMStoreFloat4x4(&m_meshConstsBufferData.world, world);

	world.r[3] = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);

	// ����� ���
	XMMATRIX worldInv = XMMatrixInverse(nullptr, world);

	// ������� ��ġ ���
	XMMATRIX worldInvTranspose = XMMatrixTranspose(worldInv);

	XMStoreFloat4x4(&m_meshConstsBufferData.worldIT, worldInvTranspose);

	memcpy(m_meshConstsBufferDataBegin, &m_meshConstsBufferData, sizeof(m_meshConstsBufferData));
}

void Model::Render(
	ComPtr<ID3D12Device> device,
	ComPtr<ID3D12GraphicsCommandList> commandList)
{

	for (const auto& mesh : m_meshes)
	{
		commandList->IASetVertexBuffers(0, 1, &mesh->vertexBufferView);
		commandList->IASetIndexBuffer(&mesh->indexBufferView);
		commandList->DrawIndexedInstanced(mesh->indexBufferCount, 1, 0, 0, 0);
	}
}