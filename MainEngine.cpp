#include "MainEngine.h"

namespace EngineCore
{
	MainEngine::MainEngine() : EngineBase()
	{
	}

	void MainEngine::Initialize()
	{
		LoadPipeline();

		Model m(m_device, m_commandList, m_basicHeap);
		models.push_back(m);

		ThrowIfFailed(m_commandList->Close());

		ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
		m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		m_fenceValue[m_frameIndex] = 1;
		WaitForPreviousFrame();

		LoadGUI();
	}

	void MainEngine::Update(float dt)
	{
		m_camera.Update(m_mouseDeltaX, m_mouseDeltaY, dt, m_isMouseMove);

		XMFLOAT4 pos = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
		XMVECTOR posVec = XMLoadFloat4(&pos);

		XMStoreFloat4x4(&m_constantBufferData.world,
			XMMatrixTranspose(XMMatrixTranslationFromVector(posVec)));

		XMStoreFloat4x4(&m_constantBufferData.view, XMMatrixTranspose(m_camera.GetViewMatrix()));

		XMStoreFloat4x4(&m_constantBufferData.proj,
			XMMatrixTranspose(m_camera.GetProjectionMatrix(45.0f * (3.14f / 180.0f), m_aspectRatio, 0.1f, 1000.0f)));

		for (int i = 0; i < models.size(); i++)
		{
			models[i].Update(m_constantBufferData.world, m_constantBufferData.view, m_constantBufferData.proj, m_constantBufferData.offset.x);
		}
	}
	
	void MainEngine::UpdateGUI()
	{
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();

		ImGui::NewFrame();

		ImGui::Begin("Scene Control");
		ImGui::Text("Average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::SliderFloat("x pos", &m_constantBufferData.offset.x, -m_aspectRatio, m_aspectRatio);

		ImGui::End();

		// Rendering
		ImGui::Render();
	}

	void MainEngine::Render()
	{
		ThrowIfFailed(m_commandAllocator[m_frameIndex]->Reset());
		ThrowIfFailed(m_commandList->Reset(m_commandAllocator[m_frameIndex].Get(), m_psoManager.m_defaultPSO.Get()));

		auto presentToRT = CD3DX12_RESOURCE_BARRIER::Transition(
			m_renderTargets[m_frameIndex].Get(),
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		m_commandList->ResourceBarrier(1, &presentToRT);

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
		CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvHeap->GetCPUDescriptorHandleForHeapStart());

		const float blackColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		m_commandList->ClearRenderTargetView(rtvHandle, blackColor, 0, nullptr);
		m_commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);;

		m_commandList->SetGraphicsRootSignature(m_psoManager.m_rootSignature.Get());
		m_commandList->RSSetViewports(1, &m_viewport);
		m_commandList->RSSetScissorRects(1, &m_scissorRect);
		m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		for (int i = 0; i < models.size(); i++)
		{
			models[i].Render(m_device, m_commandList, m_basicHeap);
		}

		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_commandList.Get());

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

}