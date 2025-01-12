#include "MainEngine.h"

MainEngine::MainEngine() : EngineBase() {}

void MainEngine::Initialize()
{
	LoadPipeline();

	CreateConstUploadBuffer(m_device, m_commandList, m_globalConstsUploadHeap, m_globalConstsBufferData, m_globalConstsBufferDataBegin);
	CreateConstUploadBuffer(m_device, m_commandList, m_cubemapIndexConstsUploadHeap, m_cubemapIndexConstsBufferData, m_cubemapIndexConstsBufferDataBegin);

	textureManager.SetTextureHandle(m_textureHeap);

	{
		MeshData skybox = GeometryGenerator::MakeBox(100.0f);
		std::reverse(skybox.indices.begin(), skybox.indices.end());

		skybox.ddsAmbientFilename = "./Assets/park_ambient.dds";
		skybox.ddsDiffuseFilename = "./Assets/park_diffuse.dds";
		skybox.ddsSpecularFilename = "./Assets/park_specular.dds";
		m_skybox = make_shared<Model>(
			m_device, m_commandList, m_commandQueue,
			vector{ skybox }, m_cubemapIndexConstsBufferData, textureManager);
		m_skybox->key = "skybox";
	}

	{
		MeshData meshData = GeometryGenerator::MakeBox(1.0f);
		meshData.diffuseFilename = "./Assets/wall_black.jpg";
		shared_ptr<Model> box = make_shared<Model>(
			m_device, m_commandList, m_commandQueue,
			vector{ meshData }, m_cubemapIndexConstsBufferData, textureManager);
		box->key = "box";
		m_models.insert({ box->key, box });
	}

	{
		MeshData meshData = GeometryGenerator::MakeSphere(1.0f, 100, 100);
		meshData.diffuseFilename = "./Assets/earth.jpg";
		shared_ptr<Model> sphere = make_shared<Model>(
			m_device, m_commandList, m_commandQueue,
			vector{ meshData }, m_cubemapIndexConstsBufferData, textureManager);
		sphere->key = "sphere";
		m_models.insert({ sphere->key, sphere });
	}

	{
		MeshData meshData = GeometryGenerator::MakeSquare();
		shared_ptr<Model> square = make_shared<Model>(
			m_device, m_commandList, m_commandQueue,
			vector{ meshData }, m_cubemapIndexConstsBufferData, textureManager);
		square->key = "square";
		m_models.insert({ square->key, square });
	}

	const UINT bloomLevels = 5;
	for (int i = 0; i < FrameCount; i++)
		m_postProcess[i] = make_shared<PostProcess>(
			m_device, m_commandList, m_width, m_height, bloomLevels);

	ThrowIfFailed(m_commandList->Close());

	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	m_fenceValue[m_frameIndex] = 1;
	WaitForPreviousFrame();

	LoadGUI();
}

void MainEngine::Update(float dt)
{
	m_camera->Update(m_mouseDeltaX, m_mouseDeltaY, dt, m_isMouseMove);

	XMMATRIX view = m_camera->GetViewMatrix();
	XMMATRIX viewTrans = XMMatrixTranspose(view);
	XMStoreFloat4x4(&m_globalConstsBufferData.view, viewTrans);

	XMMATRIX proj = m_camera->GetProjectionMatrix(XMConvertToRadians(45.0f), m_aspectRatio, 0.1f, 1000.0f);
	XMMATRIX projTrans = XMMatrixTranspose(proj);
	XMStoreFloat4x4(&m_globalConstsBufferData.proj, projTrans);

	XMVECTOR det;
	XMMATRIX invView = XMMatrixInverse(&det, view);
	XMVECTOR eyeWorld = XMVector3TransformCoord(XMVECTOR{ 0.0f, 0.0f, 0.0f }, invView);
	XMStoreFloat3(&m_globalConstsBufferData.eyeWorld, eyeWorld);

	m_globalConstsBufferData.isUseTexture = guiState.isUseTextrue;

	for (int i = 0; i < MAX_LIGHTS; i++)
	{
		if (i != m_lightType)
			m_globalConstsBufferData.lights[i].strength = { 0.0f, 0.0f, 0.0f };
		else
			m_globalConstsBufferData.lights[i] = m_lightFromGUI;
	}

	memcpy(m_globalConstsBufferDataBegin, &m_globalConstsBufferData, sizeof(m_globalConstsBufferData));
	memcpy(m_cubemapIndexConstsBufferDataBegin, &m_cubemapIndexConstsBufferData, sizeof(m_cubemapIndexConstsBufferData));

	for (const auto& model : m_models)
	{
		model.second->Update();
	}

	m_skybox->Update();

	if (dirtyFlag.isPostProcessFlag)
	{
		dirtyFlag.isPostProcessFlag = false;
		for (int i = 0; i < FrameCount; i++)
			m_postProcess[i]->Update(threshold, strength, m_frameIndex);
	}
}

void MainEngine::UpdateGUI()
{
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();

	ImGui::NewFrame();

	ImGui::Begin("Scene Control");
	ImGui::Text("Average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	ImGui::Checkbox("Draw Normals", &guiState.isDrawNormals);
	ImGui::Checkbox("Wireframe", &guiState.isWireframe);
	ImGui::Checkbox("Use Texture", &guiState.isUseTextrue);

	for (const auto& model : m_models)
	{
		ImGui::PushID(model.first.c_str());

		//ImGui::SetNextItemOpen(true, ImGuiCond_Once);

		if (ImGui::CollapsingHeader(model.first.c_str(), true)) {
			ImGui::SliderFloat3("Ambient", &model.second.get()->m_meshConstsBufferData.material.ambient.x, 0.0f, 1.0f, "%.1f");
			ImGui::SliderFloat3("Diffuse", &model.second.get()->m_meshConstsBufferData.material.diffuse.x, 0.0f, 1.0f, "%.1f");
			ImGui::SliderFloat3("Specular", &model.second.get()->m_meshConstsBufferData.material.specular.x, 0.0f, 1.0f, "%.1f");
			ImGui::SliderFloat("Shininess", &model.second.get()->m_meshConstsBufferData.material.shininess, 0.0f, 1.0f, "%.1f");

			ImGui::SliderFloat("X", &model.second.get()->pos.x, -10.0f, 10.0f, "%.1f");
			ImGui::SliderFloat("Y", &model.second.get()->pos.y, -10.0f, 10.0f, "%.1f");
			ImGui::SliderFloat("Z", &model.second.get()->pos.z, -10.0f, 10.0f, "%.1f");
		}

		ImGui::PopID();
	}
	
	if (ImGui::SliderFloat("threshold", &threshold, 0.0f, 1.0f, "%.1f"))
	{
		dirtyFlag.isPostProcessFlag = true;
	}

	if (ImGui::SliderFloat("strength", &strength, 0.0f, 3.0f, "%.1f"))
	{
		dirtyFlag.isPostProcessFlag = true;
	}

	ImGui::End();

	// Rendering
	ImGui::Render();
}

void MainEngine::Render()	
{
	ThrowIfFailed(m_commandAllocator[m_frameIndex]->Reset());
	ThrowIfFailed(m_commandList->Reset(m_commandAllocator[m_frameIndex].Get(), nullptr));

	auto presentToRT = CD3DX12_RESOURCE_BARRIER::Transition(
		m_renderTargets[m_frameIndex].Get(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_commandList->ResourceBarrier(1, &presentToRT);

	m_commandList->RSSetViewports(1, &m_viewport);
	m_commandList->RSSetScissorRects(1, &m_scissorRect);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
	m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

	const float color[] = { 0.0f, 0.2f, 1.0f, 1.0f };
	m_commandList->ClearRenderTargetView(rtvHandle, color, 0, nullptr);
	m_commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	m_commandList->SetGraphicsRootSignature(Graphics::rootSignature.Get());

	ID3D12DescriptorHeap* ppHeaps[] = { m_textureHeap.Get() };
	m_commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
	m_commandList->SetGraphicsRootConstantBufferView(0, m_globalConstsUploadHeap.Get()->GetGPUVirtualAddress());
	m_commandList->SetGraphicsRootConstantBufferView(3, m_cubemapIndexConstsUploadHeap.Get()->GetGPUVirtualAddress());

	m_skybox->RenderSkybox(m_device, m_commandList, m_textureHeap, guiState);

	for (const auto& model : m_models)
		model.second->Render(m_device, m_commandList, m_textureHeap, guiState);

	auto RenderToResource = CD3DX12_RESOURCE_BARRIER::Transition(
		m_renderTargets[m_frameIndex].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	m_commandList->ResourceBarrier(1, &RenderToResource);

	// PostProcess
	m_postProcess[m_frameIndex]->Render(m_device, m_commandList, m_renderTargets[m_frameIndex],
		m_rtvHeap, m_srvHeap, m_dsvHeap, m_frameIndex);

	ID3D12DescriptorHeap* imguiHeap[] = { m_imguiHeap.Get() };
	m_commandList->SetDescriptorHeaps(_countof(imguiHeap), imguiHeap);
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_commandList.Get());

	auto RenderToPresent = CD3DX12_RESOURCE_BARRIER::Transition(
		m_renderTargets[m_frameIndex].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	m_commandList->ResourceBarrier(1, &RenderToPresent);

	ThrowIfFailed(m_commandList->Close());

	// Execute the command list.
	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// Present the frame.
	ThrowIfFailed(m_swapChain->Present(0, 0));

	WaitForPreviousFrame();
}
