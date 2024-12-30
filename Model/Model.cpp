#include "Model.h"

Model::Model(
	ComPtr<ID3D12Device> device,
	ComPtr<ID3D12GraphicsCommandList> commandList,
	CD3DX12_CPU_DESCRIPTOR_HANDLE basicHandle,
	MeshData &meshData)
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
	MeshData &meshData)
{
	// Create the vertex buffer.
	{
		const UINT vertexBufferSizeInBytes =
			static_cast<UINT>(meshData.vertices.size() * sizeof(Vertex));

		auto buffer = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSizeInBytes);

		auto defaultHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		ThrowIfFailed(device->CreateCommittedResource(
			&defaultHeapProps,
			D3D12_HEAP_FLAG_NONE,
			&buffer,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&m_vertexBuffer)));

		m_vertexBuffer->SetName(L"Vertex Buffer Resource Heap");

		auto uploadHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		ThrowIfFailed(device->CreateCommittedResource(
			&uploadHeapProps,
			D3D12_HEAP_FLAG_NONE, // no flags
			&buffer,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_vertexUploadHeap)));

		m_vertexUploadHeap->SetName(L"Vertex Buffer Upload Resource Heap");

		D3D12_SUBRESOURCE_DATA vertexData = {};
		vertexData.pData = meshData.vertices.data();
		vertexData.RowPitch = vertexBufferSizeInBytes;
		vertexData.SlicePitch = vertexBufferSizeInBytes;

		UpdateSubresources(commandList.Get(), m_vertexBuffer.Get(), m_vertexUploadHeap.Get(), 0, 0, 1, &vertexData);

		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			m_vertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
		commandList->ResourceBarrier(1, &barrier);

		m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
		m_vertexBufferView.StrideInBytes = sizeof(Vertex);
		m_vertexBufferView.SizeInBytes = vertexBufferSizeInBytes;
	}

	// Create Index Buffer
	{
		indexBufferCount = meshData.indices.size();

		const UINT indexBufferSizeInBytes =
			static_cast<UINT>(meshData.indices.size() * sizeof(uint32_t));

		auto defaultHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		auto buffer = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSizeInBytes);
		ThrowIfFailed(device->CreateCommittedResource(
			&defaultHeapProps,
			D3D12_HEAP_FLAG_NONE, // no flags
			&buffer,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&m_indexBuffer)));

		m_indexBuffer->SetName(L"Index Buffer Resource Heap");

		auto uploadHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		ThrowIfFailed(device->CreateCommittedResource(
			&uploadHeapProps,
			D3D12_HEAP_FLAG_NONE,
			&buffer,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_indexUploadHeap)));

		m_indexUploadHeap->SetName(L"Index Buffer Upload Resource Heap");

		D3D12_SUBRESOURCE_DATA indexData = {};
		indexData.pData = meshData.indices.data();
		indexData.RowPitch = indexBufferSizeInBytes;
		indexData.SlicePitch = indexBufferSizeInBytes;

		UpdateSubresources(commandList.Get(), m_indexBuffer.Get(), m_indexUploadHeap.Get(), 0, 0, 1, &indexData);

		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			m_indexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);
		commandList->ResourceBarrier(1, &barrier);

		m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
		m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
		m_indexBufferView.SizeInBytes = indexBufferSizeInBytes;
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
		cbvDesc.SizeInBytes = (meshConstantsSize + 255) & ~255; // 256-byte 정렬
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
			m_texture.Get(), // 텍스처 리소스
			D3D12_RESOURCE_STATE_COPY_DEST, // 이전 상태
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE // 새로운 상태
		);
		commandList->ResourceBarrier(1, &barrier);

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = metaData.format; // 텍스처의 포맷
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = static_cast<UINT>(metaData.mipLevels);

		device->CreateShaderResourceView(
			m_texture.Get(), // 텍스처 리소스
			&srvDesc, // SRV 설명
			basicHandle // 디스크립터 힙의 핸들
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

	// 역행렬 계산
	XMMATRIX worldInv = XMMatrixInverse(nullptr, world);

	// 역행렬의 전치 계산
	XMMATRIX worldInvTranspose = XMMatrixTranspose(worldInv);

	XMStoreFloat4x4(&m_meshConstsBufferData.worldIT, worldInvTranspose);

	memcpy(m_meshConstsBufferDataBegin, &m_meshConstsBufferData, sizeof(m_meshConstsBufferData));
}

void Model::Render(
	ComPtr<ID3D12Device> device,
	ComPtr<ID3D12GraphicsCommandList> commandList)
{

	commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	commandList->IASetIndexBuffer(&m_indexBufferView);
	commandList->DrawIndexedInstanced(indexBufferCount, 1, 0, 0, 0);
}