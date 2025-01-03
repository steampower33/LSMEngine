#include "MainEngine.h"

MainEngine::MainEngine() : EngineBase()
{
}

void MainEngine::Initialize()
{
	LoadPipeline();

	m_textureHandle = m_textureHeap->GetCPUDescriptorHandleForHeapStart();

	/*MeshData meshData = GeometryGenerator::MakeBox();
	meshData.diffuseFilename = "./Assets/wall_black.jpg";
	shared_ptr<Model> box = make_shared<Model>(m_device, m_commandList, m_textureHandle, std::vector{ meshData });
	models.push_back(box);*/

	vector<MeshData> zeldaFbx =
		GeometryGenerator::ReadFromFile("c:/zelda/", "zeldaPosed001.fbx");

	shared_ptr<Model> zelda = make_shared<Model>(m_device, m_commandList, m_textureHandle, zeldaFbx);
	models.push_back(zelda);

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

	// 1. GetViewMatrix() 결과를 가져옴
	XMMATRIX view = m_camera.GetViewMatrix();

	// 2. Transpose 적용
	XMMATRIX viewTrans = XMMatrixTranspose(view);

	// 3. XMFLOAT4X4로 저장
	XMStoreFloat4x4(&m_globalConstsBufferData.view, viewTrans);

	// 1. GetViewMatrix() 결과를 가져옴
	XMMATRIX proj = m_camera.GetProjectionMatrix(XMConvertToRadians(45.0f), m_aspectRatio, 0.1f, 1000.0f);

	// 2. Transpose 적용
	XMMATRIX projTrans = XMMatrixTranspose(proj);

	// 3. XMFLOAT4X4로 저장
	XMStoreFloat4x4(&m_globalConstsBufferData.proj, projTrans);

	memcpy(m_globalConstsBufferDataBegin, &m_globalConstsBufferData, sizeof(m_globalConstsBufferData));

	for (const auto& model : models)
	{
		model->Update();
	}
}

void MainEngine::UpdateGUI()
{
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();

	ImGui::NewFrame();

	ImGui::Begin("Scene Control");
	ImGui::Text("Average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

	ImGui::End();

	// Rendering
	ImGui::Render();
}

void MainEngine::Render()
{
	ThrowIfFailed(m_commandAllocator[m_frameIndex]->Reset());
	ThrowIfFailed(m_commandList->Reset(m_commandAllocator[m_frameIndex].Get(), Graphics::defaultPSO.Get()));

	auto presentToRT = CD3DX12_RESOURCE_BARRIER::Transition(
		m_renderTargets[m_frameIndex].Get(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_commandList->ResourceBarrier(1, &presentToRT);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvHeap->GetCPUDescriptorHandleForHeapStart());

	const float color[] = { 0.0f, 0.2f, 1.0f, 1.0f };
	m_commandList->ClearRenderTargetView(rtvHandle, color, 0, nullptr);
	m_commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	m_commandList->SetGraphicsRootSignature(Graphics::rootSignature.Get());
	m_commandList->RSSetViewports(1, &m_viewport);
	m_commandList->RSSetScissorRects(1, &m_scissorRect);
	m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
	ID3D12DescriptorHeap* ppHeaps[] = { m_textureHeap.Get() };

	m_commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	m_commandList->SetGraphicsRootConstantBufferView(0, m_globalConstsUploadHeap.Get()->GetGPUVirtualAddress());

	for (const auto& model : models)
	{
		model->Render(m_device, m_commandList, m_textureHeap);
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
