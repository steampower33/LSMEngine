#include "MainEngine.h"

namespace EngineCore
{
	MainEngine::MainEngine() : EngineBase() {}

	void MainEngine::Initialize()
	{
		EngineBase::Initialize();


	}

	void MainEngine::Update()
	{
		memcpy(m_pCbvDataBegin, &m_constantBufferData, sizeof(m_constantBufferData));
	}

	void MainEngine::Render()
	{
		ThrowIfFailed(m_commandAllocator[m_frameIndex]->Reset());
		ThrowIfFailed(m_commandList->Reset(m_commandAllocator[m_frameIndex].Get(), m_pipelineState.Get()));

		auto presentToRT = CD3DX12_RESOURCE_BARRIER::Transition(
			m_renderTargets[m_frameIndex].Get(),
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		m_commandList->ResourceBarrier(1, &presentToRT);

		RenderScene();

		auto RTToPresent = CD3DX12_RESOURCE_BARRIER::Transition(
			m_renderTargets[m_frameIndex].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		m_commandList->ResourceBarrier(1, &RTToPresent);

		ThrowIfFailed(m_commandList->Close());

		// Execute the command list.
		ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
		m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		// Present the frame.
		ThrowIfFailed(m_swapChain->Present(0, 0));

		WaitForPreviousFrame();
	}

	void MainEngine::UpdateGUI()
	{
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();

		ImGui::NewFrame();

		ImGui::SetNextWindowPos(ImVec2(5, 5)); // (x, y)�� ȭ���� ���� ��ǥ
		ImGui::Begin("Scene Control");
		ImGui::Text("Average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::SliderFloat("x pos", &m_constantBufferData.offset.x, -m_aspectRatio, m_aspectRatio);

		ImGui::End();

		UpdateSceneViewer();

		// Rendering
		ImGui::Render();
	}

	void MainEngine::UpdateSceneViewer()
	{
		ImGui::SetNextWindowPos(ImVec2(300, 5)); // (x, y)�� ȭ���� ���� ��ǥ
		ImGui::Begin("Scene 1");

		ImVec2 currentSize = ImGui::GetWindowSize();
		// ũ�� ���� ���� Ȯ��
		if (currentSize.x != m_sceneSize.x || currentSize.y != m_sceneSize.y) {
			m_sceneSize = currentSize; // ������Ʈ
			m_aspectRatio = static_cast<float>(m_sceneSize.x) / static_cast<float>(m_sceneSize.y);

			WaitForPreviousFrame();

			for (UINT n = 0; n < FrameCount; n++)
			{
				m_sceneRenderTargets[n].Reset();
				m_fenceValue[n] = m_fenceValue[m_frameIndex];
			}

			// Reset the frame index to the current back buffer index.
			m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

			m_viewport.Width = m_sceneSize.x;
			m_viewport.Height = m_sceneSize.y;

			// Create frame resources.
			CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_sceneRTVHeap->GetCPUDescriptorHandleForHeapStart());
			CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(m_sceneSRVHeap->GetCPUDescriptorHandleForHeapStart());
			for (int n = 0; n < FrameCount; n++)
			{
				D3D12_RESOURCE_DESC renderTargetDesc = {};
				renderTargetDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; // 2���� �ؽ�ó
				renderTargetDesc.Width = m_sceneSize.x;  // �ؽ�ó�� �ʺ�
				renderTargetDesc.Height = m_sceneSize.y; // �ؽ�ó�� ����
				renderTargetDesc.DepthOrArraySize = 1; // ���� �ؽ�ó
				renderTargetDesc.MipLevels = 1; // MipMap ����
				renderTargetDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // ����
				renderTargetDesc.SampleDesc.Count = 1; // ��Ƽ���ø� ��Ȱ��ȭ
				renderTargetDesc.SampleDesc.Quality = 0;
				renderTargetDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
				renderTargetDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET; // ���� Ÿ�� �÷���

				D3D12_CLEAR_VALUE clearValue = {};
				clearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				clearValue.Color[0] = 0.0f;
				clearValue.Color[1] = 0.0f;
				clearValue.Color[2] = 0.0f;
				clearValue.Color[3] = 1.0f;

				auto defaultHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
				ThrowIfFailed(m_device->CreateCommittedResource(
					&defaultHeapProps,
					D3D12_HEAP_FLAG_NONE,
					&renderTargetDesc,
					D3D12_RESOURCE_STATE_RENDER_TARGET,
					&clearValue,
					IID_PPV_ARGS(&m_sceneRenderTargets[n])
				));

				// RTV ����
				m_device->CreateRenderTargetView(m_sceneRenderTargets[n].Get(), nullptr, rtvHandle);
				rtvHandle.Offset(1, m_rtvDescriptorSize);

				// SRV ����
				D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
				srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
				srvDesc.Texture2D.MipLevels = 1;
				srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

				m_device->CreateShaderResourceView(m_sceneRenderTargets[n].Get(), &srvDesc, srvHandle);
				srvHandle.Offset(1, m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
			}
		}

		UINT srvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		D3D12_GPU_DESCRIPTOR_HANDLE srvHandle = m_sceneSRVHeap->GetGPUDescriptorHandleForHeapStart();
		srvHandle.ptr += m_frameIndex * srvDescriptorSize;
		ImVec2 contentSize = ImGui::GetContentRegionAvail(); // â ���� ���� ���� Ȯ��
		ImGui::Image(srvHandle.ptr, contentSize);

		ImGui::End();
	}

	void MainEngine::RenderScene()
	{

		m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());

		// Set DescriptorHeap
		ID3D12DescriptorHeap* ppHeaps[] = { m_basicHeap.Get() };
		m_commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

		D3D12_GPU_DESCRIPTOR_HANDLE cbvGPUHandle(m_basicHeap->GetGPUDescriptorHandleForHeapStart());
		m_commandList->SetGraphicsRootDescriptorTable(0, cbvGPUHandle);

		CD3DX12_GPU_DESCRIPTOR_HANDLE srvGpuHandle(
			m_basicHeap->GetGPUDescriptorHandleForHeapStart(),
			1, // 1�� �ε��� (CBV ����)
			m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
		);
		m_commandList->SetGraphicsRootDescriptorTable(1, srvGpuHandle);


		CD3DX12_CPU_DESCRIPTOR_HANDLE viewRTVHandle(m_sceneRTVHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
		const float whiteColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		m_commandList->ClearRenderTargetView(viewRTVHandle, whiteColor, 0, nullptr);
		m_commandList->OMSetRenderTargets(1, &viewRTVHandle, FALSE, nullptr);
		m_commandList->RSSetViewports(1, &m_viewport);
		m_commandList->RSSetScissorRects(1, &m_scissorRect);
		m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
		m_commandList->IASetIndexBuffer(&m_indexBufferView);
		m_commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);

		CD3DX12_RESOURCE_BARRIER viewRTToSR = CD3DX12_RESOURCE_BARRIER::Transition(
			m_sceneRenderTargets[m_frameIndex].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,        // ���� ����
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE // ���� ����
		);
		m_commandList->ResourceBarrier(1, &viewRTToSR);

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);

		const float blackColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		m_commandList->ClearRenderTargetView(rtvHandle, blackColor, 0, nullptr);
		m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_commandList.Get());

		CD3DX12_RESOURCE_BARRIER viewSRToRT = CD3DX12_RESOURCE_BARRIER::Transition(
			m_sceneRenderTargets[m_frameIndex].Get(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_RENDER_TARGET
		);
		m_commandList->ResourceBarrier(1, &viewSRToRT);
	}
}