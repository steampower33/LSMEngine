
#include "Model.h"

namespace Renderer
{
	Model::Model(
		ComPtr<ID3D12Device> device,
		ComPtr<ID3D12GraphicsCommandList> commandList,
		ComPtr<ID3D12DescriptorHeap> heap)
	{
		Initialize(device, commandList, heap);
	}

	Model::~Model()
	{

	}

	void Model::Initialize(
		ComPtr<ID3D12Device> device,
		ComPtr<ID3D12GraphicsCommandList> commandList,
		ComPtr<ID3D12DescriptorHeap> heap)
	{
		// Create the vertex buffer.
		{
			Vertex vertexList[] =
			{
				// front
				{ -1.0f, -1.0f, -1.0f, 0.0f, 1.0f },
				{ -1.0f,  1.0f, -1.0f, 0.0f, 0.0f },
				{  1.0f,  1.0f, -1.0f, 1.0f, 0.0f },
				{  1.0f, -1.0f, -1.0f, 1.0f, 1.0f },
			};

			int vertexBufferSize = sizeof(vertexList);

			auto buffer = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);

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
			vertexData.pData = reinterpret_cast<BYTE*>(vertexList);
			vertexData.RowPitch = vertexBufferSize;
			vertexData.SlicePitch = vertexBufferSize;

			UpdateSubresources(commandList.Get(), m_vertexBuffer.Get(), m_vertexUploadHeap.Get(), 0, 0, 1, &vertexData);

			auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
				m_vertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
			commandList->ResourceBarrier(1, &barrier);

			m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
			m_vertexBufferView.StrideInBytes = sizeof(Vertex);
			m_vertexBufferView.SizeInBytes = vertexBufferSize;
		}

		// Create Index Buffer
		{
			DWORD indexList[] = {
				0, 1, 2,
				0, 2, 3
			};

			int indexBufferSize = sizeof(indexList);

			auto defaultHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
			auto buffer = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);
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
			indexData.pData = reinterpret_cast<BYTE*>(indexList);
			indexData.RowPitch = indexBufferSize;
			indexData.SlicePitch = indexBufferSize;

			UpdateSubresources(commandList.Get(), m_indexBuffer.Get(), m_indexUploadHeap.Get(), 0, 0, 1, &indexData);

			auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
				m_indexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);
			commandList->ResourceBarrier(1, &barrier);

			m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
			m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
			m_indexBufferView.SizeInBytes = indexBufferSize;
		}

		CD3DX12_CPU_DESCRIPTOR_HANDLE handle(heap->GetCPUDescriptorHandleForHeapStart());

		{
			const UINT constantBufferSize = sizeof(SceneConstantBuffer);

			auto uploadHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
			auto buffer = CD3DX12_RESOURCE_DESC::Buffer(constantBufferSize);
			ThrowIfFailed(device->CreateCommittedResource(
				&uploadHeapProps,
				D3D12_HEAP_FLAG_NONE,
				&buffer,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&m_constantBuffer)));

			CD3DX12_RANGE readRange(0, 0);
			ThrowIfFailed(m_constantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_pCbvDataBegin)));
			memcpy(m_pCbvDataBegin, &m_constantBufferData, sizeof(m_constantBufferData));

			// Describe and create a constant buffer view.
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
			cbvDesc.BufferLocation = m_constantBuffer->GetGPUVirtualAddress();
			cbvDesc.SizeInBytes = (constantBufferSize + 255) & ~255; // 256-byte 정렬
			device->CreateConstantBufferView(&cbvDesc, handle);

			handle.Offset(1, device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
		}


		// Create the texture.
		auto image = std::make_unique<ScratchImage>();
		{
			ThrowIfFailed(DirectX::LoadFromWICFile(L"wall.jpg", DirectX::WIC_FLAGS_NONE, nullptr, *image));

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
				handle // 디스크립터 힙의 핸들
			);

		}
	}

	void Model::Update(const XMFLOAT4X4 &world, const XMFLOAT4X4& view, const XMFLOAT4X4& proj)
	{
		m_constantBufferData.world = world;
		m_constantBufferData.view = view;
		m_constantBufferData.proj = proj;
		memcpy(m_pCbvDataBegin, &m_constantBufferData, sizeof(m_constantBufferData));
	}

	void Model::Render(
		ComPtr<ID3D12Device> device,
		ComPtr<ID3D12GraphicsCommandList> commandList,
		ComPtr<ID3D12DescriptorHeap> heap)
	{
		// Set DescriptorHeap
		ID3D12DescriptorHeap* ppHeaps[] = { heap.Get() };
		commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

		D3D12_GPU_DESCRIPTOR_HANDLE cbvGPUHandle(heap->GetGPUDescriptorHandleForHeapStart());
		commandList->SetGraphicsRootDescriptorTable(0, cbvGPUHandle);

		CD3DX12_GPU_DESCRIPTOR_HANDLE srvGpuHandle(
			heap->GetGPUDescriptorHandleForHeapStart(),
			1, // 1번 인덱스 (CBV 이후)
			device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
		);
		commandList->SetGraphicsRootDescriptorTable(1, srvGpuHandle);

		commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
		commandList->IASetIndexBuffer(&m_indexBufferView);
		commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);
	}
}