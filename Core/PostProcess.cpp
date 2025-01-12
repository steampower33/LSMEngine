#include "PostProcess.h"

PostProcess::PostProcess(
    ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& commandList,
    float width, float height, const int bloomLevels)
{
    m_bloomLevels = bloomLevels;
    Initialize(device, commandList, width, height);
}

PostProcess::~PostProcess()
{

}

void PostProcess::Initialize(
	ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& commandList,
    float width, float height
	)
{

	MeshData square = GeometryGenerator::MakeSquare();

    m_mesh = make_shared<Mesh>();
    m_mesh->vertexBufferCount = square.vertices.size();
    m_mesh->indexBufferCount = square.indices.size();
    CreateVertexBuffer(device, commandList, square.vertices, m_mesh);
	CreateIndexBuffer(device, commandList, square.indices, m_mesh);

    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = m_bloomLevels;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    ThrowIfFailed(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));

    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
    srvHeapDesc.NumDescriptors = m_bloomLevels;
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ThrowIfFailed(device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_srvHeap)));

    m_textures.resize(m_bloomLevels);
    for (int i = 0; i < m_bloomLevels; i++)
    {
        float div = float(pow(2, i + 1));
        float newWidth = width / div;
        float newHeight = height / div;

        shared_ptr<ImageFilter> downFilter = make_shared<ImageFilter>(device, commandList, newWidth, newHeight, i - 1);
        m_filters.push_back(downFilter);

        CreateTex2D(device, m_textures[i], newWidth, newHeight, i, m_rtvHeap, m_srvHeap);
    }

    for (int i = m_bloomLevels - 1; i > 0; i--)
    {
        float div = float(pow(2, i));
        float newWidth = width / div;
        float newHeight = height / div;

        shared_ptr<ImageFilter> upFilter = make_shared<ImageFilter>(device, commandList, newWidth, newHeight, i);
        m_filters.push_back(upFilter);
    }

    m_combineFilter = make_shared<ImageFilter>(device, commandList, width, height, 0);
}

void PostProcess::Update(UINT frameIndex)
{
}

void PostProcess::Render(
    ComPtr<ID3D12Device>& device,
    ComPtr<ID3D12GraphicsCommandList>& commandList,
    ComPtr<ID3D12Resource>& renderTarget,
    ComPtr<ID3D12DescriptorHeap>& rtv,
    ComPtr<ID3D12DescriptorHeap>& srv,
    ComPtr<ID3D12DescriptorHeap>& dsv,
    UINT frameIndex)
{
    commandList->IASetVertexBuffers(0, 1, &m_mesh->vertexBufferView);
    commandList->IASetIndexBuffer(&m_mesh->indexBufferView);

    commandList->SetPipelineState(Graphics::filterPSO.Get());
    
    // DownSampling
    for (int i = 0; i < m_bloomLevels; i++)
    {
        if (i == 0)
        {
            ID3D12DescriptorHeap* heaps[] = { srv.Get() };
            commandList->SetDescriptorHeaps(_countof(heaps), heaps);

            CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle(srv->GetGPUDescriptorHandleForHeapStart());
            commandList->SetGraphicsRootDescriptorTable(6, srvHandle);
            m_filters[0]->Update(frameIndex);
        }
        else if (i == 1)
        {
            ID3D12DescriptorHeap* heaps[] = { m_srvHeap.Get() };
            commandList->SetDescriptorHeaps(_countof(heaps), heaps);

            CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle(m_srvHeap->GetGPUDescriptorHandleForHeapStart());
            commandList->SetGraphicsRootDescriptorTable(6, srvHandle);
        }

        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), rtvSize * i);
        CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(dsv->GetCPUDescriptorHandleForHeapStart());
        commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
        commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
        
        m_filters[i]->Render(commandList);
    
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList->DrawIndexedInstanced(m_mesh->indexBufferCount, 1, 0, 0, 0);
        
        auto RenderToResource = CD3DX12_RESOURCE_BARRIER::Transition(
            m_textures[i].Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        commandList->ResourceBarrier(1, &RenderToResource);
    }

    // UpSampling
    for (int i = m_bloomLevels - 1; i > 0; i--)
    {
        auto ResourceToRender = CD3DX12_RESOURCE_BARRIER::Transition(
            m_textures[i - 1].Get(),
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
        commandList->ResourceBarrier(1, &ResourceToRender);

        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), rtvSize * (i - 1));
        CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(dsv->GetCPUDescriptorHandleForHeapStart());
        commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
        commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

        m_filters[m_bloomLevels + m_bloomLevels - (i + 1)]->Render(commandList);

        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList->DrawIndexedInstanced(m_mesh->indexBufferCount, 1, 0, 0, 0);

        auto RenderToResource = CD3DX12_RESOURCE_BARRIER::Transition(
            m_textures[i - 1].Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        commandList->ResourceBarrier(1, &RenderToResource);
    }

    auto ResourceToRender = CD3DX12_RESOURCE_BARRIER::Transition(
        renderTarget.Get(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
    commandList->ResourceBarrier(1, &ResourceToRender);

    commandList->SetPipelineState(Graphics::combinePSO.Get());
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvhandle(rtv->GetCPUDescriptorHandleForHeapStart(), rtvSize * frameIndex);
    CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(dsv->GetCPUDescriptorHandleForHeapStart());
    commandList->OMSetRenderTargets(1, &rtvhandle, FALSE, &dsvHandle);
    commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    m_combineFilter->Render(commandList);

    commandList->DrawIndexedInstanced(m_mesh->indexBufferCount, 1, 0, 0, 0);

    for (int i = 0; i < m_bloomLevels; i++)
    {
        auto ResourceToRender = CD3DX12_RESOURCE_BARRIER::Transition(
            m_textures[i].Get(),
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
        commandList->ResourceBarrier(1, &ResourceToRender);
    }
}

void PostProcess::CreateTex2D(
    ComPtr<ID3D12Device>& device, ComPtr<ID3D12Resource>& texture, 
    float width, float height, UINT index,
    ComPtr<ID3D12DescriptorHeap>& rtv, ComPtr<ID3D12DescriptorHeap>& srv)
{
    auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

    auto texDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        DXGI_FORMAT_R32G32B32A32_FLOAT, // 텍스처 포맷
        width,                          // 화면 너비
        height,                         // 화면 높이
        1,                              // arraySize
        1,                              // mipLevels
        1,                              // sampleCount
        0,                              // sampleQuality
        D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET // RT로 사용할 예정이면 플래그 설정
    );

    ThrowIfFailed(device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        nullptr,
        IID_PPV_ARGS(&texture)
    ));

    rtvSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    srvSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), rtvSize * index);
    CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(m_srvHeap->GetCPUDescriptorHandleForHeapStart(), srvSize * index);

    // Create a RTV for each frame
    device->CreateRenderTargetView(texture.Get(), nullptr, rtvHandle);

    // Create a SRV for each frame
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = texture->GetDesc().Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = texture->GetDesc().MipLevels;

    device->CreateShaderResourceView(
        texture.Get(),
        &srvDesc,
        srvHandle
    );
}