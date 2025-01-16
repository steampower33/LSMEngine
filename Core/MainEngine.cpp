#include "MainEngine.h"

MainEngine::MainEngine() : EngineBase() {}

void MainEngine::Initialize()
{
	LoadPipeline();

	CreateConstUploadBuffer(m_device, m_commandList, m_globalConstsUploadHeap, m_globalConstsBufferData, m_globalConstsBufferDataBegin);
	CreateConstUploadBuffer(m_device, m_commandList, m_cubemapIndexConstsUploadHeap, m_cubemapIndexConstsBufferData, m_cubemapIndexConstsBufferDataBegin);

	m_textureManager = make_shared<TextureManager>(m_device, m_commandList, m_textureHeap);

	{
		MeshData skybox = GeometryGenerator::MakeBox(100.0f);
		std::reverse(skybox.indices.begin(), skybox.indices.end());

		skybox.ddsAmbientFilename = "./Assets/park_ambient.dds";
		skybox.ddsDiffuseFilename = "./Assets/park_diffuse.dds";
		skybox.ddsSpecularFilename = "./Assets/park_specular.dds";
		m_skybox = make_shared<Model>(
			m_device, m_commandList, m_commandQueue,
			vector{ skybox }, m_cubemapIndexConstsBufferData, m_textureManager);
		m_skybox->key = "skybox";
	}

	{
		float radius = 1.0f;
		MeshData meshData = GeometryGenerator::MakeSphere(radius, 100, 100);

		meshData.diffuseFilename = "./Assets/earth_diffuse.jpg";
		shared_ptr<Model> sphere = make_shared<Model>(
			m_device, m_commandList, m_commandQueue,
			vector{ meshData }, m_cubemapIndexConstsBufferData, m_textureManager);
		sphere->key = "sphere";
		m_models.insert({ sphere->key, sphere });

		m_boundingSphere = BoundingSphere(XMFLOAT3(sphere->position.x, sphere->position.y, sphere->position.z), radius);

		MeshData cursorSphere = GeometryGenerator::MakeSphere(0.05f, 100, 100);
		m_cursorSphere = make_shared<Model>(
			m_device, m_commandList, m_commandQueue,
			vector{ cursorSphere }, m_cubemapIndexConstsBufferData, m_textureManager);
		m_cursorSphere->m_meshConstsBufferData.material.diffuse = XMFLOAT3(1.0f, 1.0f, 0.0f);
		m_cursorSphere->m_meshConstsBufferData.material.specular = XMFLOAT3(0.0f, 0.0f, 0.0f);
	}

	for (int i = 0; i < FrameCount; i++)
		m_postProcess[i] = make_shared<PostProcess>(
			m_device, m_commandList, m_width, m_height);

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
	
	XMVECTOR axis = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
	float angleRadians = 0.0f;
	XMVECTOR q = XMQuaternionRotationAxis(axis, angleRadians);
	
	static XMVECTOR prevVector{ 0.0f };
	if (m_leftButton)
	{
		// NDC ��ǥ�� Ŭ�� ������ ��ǥ�� ��ȯ (Z = 0.0f�� Near Plane, Z = 1.0f�� Far Plane)
		XMVECTOR rayNDCNear = XMVectorSet(m_ndcX, m_ndcY, 0.0f, 1.0f);
		XMVECTOR rayNDCFar = XMVectorSet(m_ndcX, m_ndcY, 1.0f, 1.0f);

		// �� �������� ��Ʈ������ ����Ͽ� ���� ������ ��ǥ�� ��ȯ
		XMMATRIX invProjection = XMMatrixInverse(nullptr, proj);
		XMMATRIX invView = XMMatrixInverse(nullptr, view);

		// Ŭ�� ������ ���� �������� ��ȯ
		XMVECTOR rayViewNear = XMVector3TransformCoord(rayNDCNear, invProjection);
		XMVECTOR rayViewFar = XMVector3TransformCoord(rayNDCFar, invProjection);

		// ���� ������ ���� �������� ��ȯ
		XMVECTOR rayWorldNear = XMVector3TransformCoord(rayViewNear, invView);
		XMVECTOR rayWorldFar = XMVector3TransformCoord(rayViewFar, invView);

		// ������ ������ ���� ���
		XMFLOAT3 originFloat;
		XMStoreFloat3(&originFloat, rayWorldNear);
		XMFLOAT3 farPointFloat;
		XMStoreFloat3(&farPointFloat, rayWorldFar);

		XMFLOAT3 direction = {
			farPointFloat.x - originFloat.x,
			farPointFloat.y - originFloat.y,
			farPointFloat.z - originFloat.z
		};

		// ���� ���� ����ȭ
		XMVECTOR directionVec = XMVector3Normalize(XMLoadFloat3(&direction));

		Ray ray(originFloat, directionVec);

		float dist = 0.0f;
		m_selected = ray.RaySphereIntersect(m_boundingSphere, dist);

		if (m_selected)
		{
			//cout << dist << endl;

			// �浹 ������ ���� �� �׸���
			XMVECTOR pickVec = XMVectorAdd(rayWorldNear, XMVectorScale(directionVec, dist));
			XMMATRIX worldMat = XMMatrixTranslationFromVector(pickVec);
			XMMATRIX invTranspose = worldMat;
			invTranspose.r[3] = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
			invTranspose = XMMatrixTranspose(XMMatrixInverse(nullptr, invTranspose));
			XMStoreFloat4x4(&m_cursorSphere->m_meshConstsBufferData.world, XMMatrixTranspose(worldMat));
			XMStoreFloat4x4(&m_cursorSphere->m_meshConstsBufferData.worldIT, invTranspose);
			m_cursorSphere->OnlyCallConstsMemcpy();

			// �巡��
			XMVECTOR centerVec = XMLoadFloat3(&m_boundingSphere.Center);
			if (m_dragStartFlag)
			{
				m_dragStartFlag = false;
				prevVector = XMVectorSubtract(pickVec, centerVec);
				prevVector = XMVector3Normalize(prevVector);
			}
			else
			{
				// ���� ���� ��� �� ����ȭ
				XMVECTOR currentVector = XMVectorSubtract(pickVec, centerVec);
				currentVector = XMVector3Normalize(currentVector);

				// ���� ���� ���
				XMVECTOR delta = XMVectorSubtract(currentVector, prevVector);
				float deltaLength = XMVectorGetX(XMVector3Length(delta));

				// ���콺�� �����̶� �������� ��쿡�� ȸ����Ű��
				if (deltaLength > 1e-3f) {
					// ��� ���� ���
					XMVECTOR cross = XMVector3Cross(prevVector, currentVector);
					XMVECTOR dot = XMVector3Dot(prevVector, currentVector);
					float dotValue = XMVectorGetX(dot);
					float angle = acosf(dotValue); // ���� (����)

					// ���Ͱ� �ݴ� ������ ��츦 ó��
					if (fabsf(dotValue + 1.0f) < 1e-3f) {
						// ���Ͱ� �ݴ� ������ ���, ������ ���� �� ����
						XMVECTOR arbitrary = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
						cross = XMVector3Cross(prevVector, arbitrary);
						cross = XMVector3Normalize(cross);
						angle = XM_PI;
					}
					else {
						// �� ����ȭ
						cross = XMVector3Normalize(cross);
					}

					// ���ʹϾ� ����
					q = XMQuaternionRotationAxis(cross, angle);

					// ���� ���� ������Ʈ
					prevVector = currentVector;
				}
			}
		}
	}

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
		model.second->Update(q);
	}

	XMVECTOR qi = XMQuaternionIdentity();
	m_skybox->Update(qi);

	m_postProcess[m_frameIndex]->UpdateIndex(m_frameIndex);

	if (dirtyFlag.isPostProcessFlag)
	{
		dirtyFlag.isPostProcessFlag = false;
		for (int i = 0; i < FrameCount; i++)
			m_postProcess[i]->Update(threshold, strength);
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

			ImGui::SliderFloat("X", &model.second.get()->position.x, -10.0f, 10.0f, "%.1f");
			ImGui::SliderFloat("Y", &model.second.get()->position.y, -10.0f, 10.0f, "%.1f");
			ImGui::SliderFloat("Z", &model.second.get()->position.z, -10.0f, 10.0f, "%.1f");
		}

		ImGui::PopID();
	}

	if (ImGui::CollapsingHeader("Post Process"))
	{
		if (ImGui::SliderFloat("threshold", &threshold, 0.0f, 1.0f, "%.1f"))
		{
			dirtyFlag.isPostProcessFlag = true;
		}

		if (ImGui::SliderFloat("strength", &strength, 0.0f, 3.0f, "%.1f"))
		{
			dirtyFlag.isPostProcessFlag = true;
		}
	}

	ImGui::End();

	// Rendering
	ImGui::Render();
}

void MainEngine::Render()
{
	ThrowIfFailed(m_commandAllocator[m_frameIndex]->Reset());
	ThrowIfFailed(m_commandList->Reset(m_commandAllocator[m_frameIndex].Get(), nullptr));

	SetBarrier(m_commandList, m_renderTargets[m_frameIndex],
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

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

	if (m_leftButton && m_selected)
		m_cursorSphere->Render(m_device, m_commandList, m_textureHeap, guiState);

	SetBarrier(m_commandList, m_renderTargets[m_frameIndex],
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	// PostProcess
	m_postProcess[m_frameIndex]->Render(m_device, m_commandList, m_renderTargets[m_frameIndex],
		m_rtvHeap, m_srvHeap, m_dsvHeap, m_frameIndex);

	ID3D12DescriptorHeap* imguiHeap[] = { m_imguiHeap.Get() };
	m_commandList->SetDescriptorHeaps(_countof(imguiHeap), imguiHeap);
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_commandList.Get());

	SetBarrier(m_commandList, m_renderTargets[m_frameIndex],
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

	ThrowIfFailed(m_commandList->Close());

	// Execute the command list.
	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// Present the frame.
	ThrowIfFailed(m_swapChain->Present(0, 0));

	WaitForPreviousFrame();
}
