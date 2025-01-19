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

		skybox.cubeEnvFilename = "./Assets/DaySky/DaySkyHDRIEnvHDR.dds";
		skybox.cubeDiffuseFilename = "./Assets/DaySky/DaySkyHDRIDiffuseHDR.dds";
		skybox.cubeSpecularFilename = "./Assets/DaySky/DaySkyHDRISpecularHDR.dds";

		m_skybox = make_shared<Model>(
			m_device, m_commandList, m_commandQueue,
			vector{ skybox }, m_cubemapIndexConstsBufferData, m_textureManager, XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f));
	}

	{
		MeshData cursorSphere = GeometryGenerator::MakeSphere(0.025f, 100, 100);
		m_cursorSphere = make_shared<Model>(
			m_device, m_commandList, m_commandQueue,
			vector{ cursorSphere }, m_cubemapIndexConstsBufferData, m_textureManager, XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f));
	}

	{
		MeshData meshData = GeometryGenerator::MakeSquare(10.0f);

		meshData.albedoFilename = "./Assets/ChessBoard_Albedo.png";
		m_board = make_shared<Model>(
			m_device, m_commandList, m_commandQueue,
			vector{ meshData }, m_cubemapIndexConstsBufferData, m_textureManager, XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f));

		float degrees = 90.0f;
		float radians = XMConvertToRadians(degrees); // DirectXMath 함수 사용
		XMVECTOR AxisX{ 1.0f, 0.0f, 0.0f, 0.0f };
		XMVECTOR quaternion = XMQuaternionRotationAxis(AxisX, radians);

		XMVECTOR translation{ 0.0f, -2.0f, 0.0f, 0.0f };
		m_board->Update(quaternion, translation);
	}

	/*{
		MeshData meshData = GeometryGenerator::MakeSquare(1.0f);
		meshData.albedoFilename = "./Assets/grey_porous_rock_40_56_Albedo.jpg";
		meshData.normalFilename = "./Assets/grey_porous_rock_40_56_Normal.jpg";
		meshData.heightFilename = "./Assets/grey_porous_rock_40_56_Height.jpg";

		shared_ptr<Model> square = make_shared<Model>(
			m_device, m_commandList, m_commandQueue,
			vector{ meshData }, m_cubemapIndexConstsBufferData, m_textureManager, XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f));
		square->m_key = "square";
		m_models.insert({ square->m_key, square });
	}*/

	{
		float radius = 1.0f;
		MeshData meshData = GeometryGenerator::MakeSphere(radius, 100, 100);
		meshData.albedoFilename = "./Assets/grey_porous_rock_40_56_Albedo.jpg";
		meshData.normalFilename = "./Assets/grey_porous_rock_40_56_Normal.jpg";
		meshData.heightFilename = "./Assets/grey_porous_rock_40_56_Height.jpg";

		shared_ptr<Model> sphere = make_shared<Model>(
			m_device, m_commandList, m_commandQueue,
			vector{ meshData }, m_cubemapIndexConstsBufferData, m_textureManager, XMFLOAT4(-2.0f, 0.0f, 0.0f, 0.0f));
		sphere->m_key = "sphere";
		m_models.insert({ sphere->m_key, sphere });
	}

	/*
	{
		MeshData meshData = GeometryGenerator::MakeBox(1.0f);
		meshData.albedoFilename = "./Assets/Bricks075A_4K-JPG_Albedo.jpg";
		meshData.normalFilename = "./Assets/Bricks075A_4K-JPG_NormalDX.jpg";

		shared_ptr<Model> box = make_shared<Model>(
			m_device, m_commandList, m_commandQueue,
			vector{ meshData }, m_cubemapIndexConstsBufferData, m_textureManager, XMFLOAT4(2.0f, 0.0f, 0.0f, 0.0f));
		box->m_key = "box";
		m_models.insert({ box->m_key, box });
	}*/

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

	UpdateMouseControl(view, proj);

	if (guiState.isMeshChanged)
	{
		guiState.isMeshChanged = false;
		m_models[guiState.changedMeshKey]->UpdateState();
	}

	m_globalConstsBufferData.lights[1] = m_lightFromGUI;

	memcpy(m_globalConstsBufferDataBegin, &m_globalConstsBufferData, sizeof(m_globalConstsBufferData));
	memcpy(m_cubemapIndexConstsBufferDataBegin, &m_cubemapIndexConstsBufferData, sizeof(m_cubemapIndexConstsBufferData));

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

	ImGui::Separator();
	ImGui::Text("Light");

	ImGui::SliderFloat3("Position", &m_lightFromGUI.position.x, -5.0f, 5.0f);

	for (const auto& model : m_models)
	{
		ImGui::PushID(model.first.c_str());

		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		if (ImGui::TreeNode(model.first.c_str())) {
			if (ImGui::SliderFloat("Albedo", &model.second.get()->m_meshConstsBufferData.material.albedo, 0.0f, 1.0f) ||
				ImGui::SliderFloat("Diffuse", &model.second.get()->m_meshConstsBufferData.material.diffuse, 0.0f, 1.0f) ||
				ImGui::SliderFloat("Specular", &model.second.get()->m_meshConstsBufferData.material.specular, 0.0f, 1.0f) ||
				ImGui::SliderFloat("Shininess", &model.second.get()->m_meshConstsBufferData.material.shininess, 0.0f, 1.0f) ||
				ImGui::Checkbox("Use Texture", &model.second.get()->m_isUseTexture) ||
				ImGui::Checkbox("Use NormalMap", &model.second.get()->m_isUseNormalMap) ||
				ImGui::Checkbox("Use HeightMap", &model.second.get()->m_isUseHeightMap) ||
				ImGui::SliderFloat("Height Scale", &model.second.get()->m_meshConstsBufferData.heightScale, 0.0f, 0.1f)
				)
			{
				guiState.isMeshChanged = true;
				guiState.changedMeshKey = model.first;
			}
			ImGui::TreePop();
		}

		ImGui::PopID();
	}

	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if (ImGui::TreeNode("Post Process"))
	{
		if (ImGui::SliderFloat("threshold", &threshold, 0.0f, 1.0f))
		{
			dirtyFlag.isPostProcessFlag = true;
		}

		if (ImGui::SliderFloat("strength", &strength, 0.0f, 3.0f))
		{
			dirtyFlag.isPostProcessFlag = true;
		}
		ImGui::TreePop();
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
	m_board->Render(m_device, m_commandList, m_textureHeap, guiState);

	for (const auto& model : m_models)
		model.second->Render(m_device, m_commandList, m_textureHeap, guiState);

	if (m_selected && (m_leftButton || m_rightButton) )
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

void MainEngine::UpdateMouseControl(XMMATRIX& view, XMMATRIX& proj)
{
	XMVECTOR q = XMQuaternionIdentity();
	XMVECTOR dragTranslation{ 0.0f };

	if (m_leftButton || m_rightButton)
	{
		// NDC 좌표를 클립 공간의 좌표로 변환 (Z = 0.0f는 Near Plane, Z = 1.0f는 Far Plane)
		XMVECTOR rayNDCNear = XMVectorSet(m_ndcX, m_ndcY, 0.0f, 1.0f);
		XMVECTOR rayNDCFar = XMVectorSet(m_ndcX, m_ndcY, 1.0f, 1.0f);

		// 역 프로젝션 매트릭스를 사용하여 월드 공간의 좌표로 변환
		XMMATRIX invProjection = XMMatrixInverse(nullptr, proj);
		XMMATRIX invView = XMMatrixInverse(nullptr, view);

		// 클립 공간을 보기 공간으로 변환
		XMVECTOR rayViewNear = XMVector3TransformCoord(rayNDCNear, invProjection);
		XMVECTOR rayViewFar = XMVector3TransformCoord(rayNDCFar, invProjection);

		// 보기 공간을 월드 공간으로 변환
		XMVECTOR rayWorldNear = XMVector3TransformCoord(rayViewNear, invView);
		XMVECTOR rayWorldFar = XMVector3TransformCoord(rayViewFar, invView);

		// 광선의 원점과 방향 계산
		XMFLOAT3 originFloat;
		XMStoreFloat3(&originFloat, rayWorldNear);
		XMFLOAT3 farPointFloat;
		XMStoreFloat3(&farPointFloat, rayWorldFar);

		XMFLOAT3 direction = {
			farPointFloat.x - originFloat.x,
			farPointFloat.y - originFloat.y,
			farPointFloat.z - originFloat.z
		};

		// 방향 벡터 정규화
		XMVECTOR directionVec = XMVector3Normalize(XMLoadFloat3(&direction));

		Ray ray(originFloat, directionVec);

		shared_ptr<Model> selectedModel = nullptr;
		float dist = 0.0f;
		for (auto& model : m_models)
		{
			m_selected = ray.RaySphereIntersect(model.second->m_boundingSphere, dist);
			if (m_selected)
			{
				selectedModel = model.second;
				break;
			}
		}

		if (m_selected)
		{
			//cout << dist << endl;

			// 충돌 지점에 작은 구 그리기
			XMVECTOR pickVec = XMVectorAdd(rayWorldNear, XMVectorScale(directionVec, dist));
			XMMATRIX worldMat = XMMatrixTranslationFromVector(pickVec);
			XMMATRIX invTranspose = worldMat;
			invTranspose.r[3] = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
			invTranspose = XMMatrixTranspose(XMMatrixInverse(nullptr, invTranspose));
			XMStoreFloat4x4(&m_cursorSphere->m_meshConstsBufferData.world, XMMatrixTranspose(worldMat));
			XMStoreFloat4x4(&m_cursorSphere->m_meshConstsBufferData.worldIT, invTranspose);
			m_cursorSphere->OnlyCallConstsMemcpy();

			// 드래그
			XMVECTOR centerVec = XMLoadFloat3(&selectedModel->m_boundingSphere->Center);

			if (m_leftButton)
			{
				static XMVECTOR prevVector{ 0.0f };
				if (m_dragStartFlag)
				{
					m_dragStartFlag = false;
					prevVector = XMVectorSubtract(pickVec, centerVec);
					prevVector = XMVector3Normalize(prevVector);
				}
				else
				{
					// 현재 벡터 계산 및 정규화
					XMVECTOR currentVector = XMVectorSubtract(pickVec, centerVec);
					currentVector = XMVector3Normalize(currentVector);

					// 벡터 차이 계산
					XMVECTOR delta = XMVectorSubtract(currentVector, prevVector);
					float deltaLength = XMVectorGetX(XMVector3Length(delta));

					// 마우스가 조금이라도 움직였을 경우에만 회전시키기
					if (deltaLength > 1e-3f) {
						// 축과 각도 계산
						XMVECTOR cross = XMVector3Cross(prevVector, currentVector);
						XMVECTOR dot = XMVector3Dot(prevVector, currentVector);
						float dotValue = XMVectorGetX(dot);
						float angle = acosf(dotValue); // 각도 (라디안)

						// 벡터가 반대 방향인 경우를 처리
						if (fabsf(dotValue + 1.0f) < 1e-3f) {
							// 벡터가 반대 방향인 경우, 임의의 직교 축 선택
							XMVECTOR arbitrary = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
							cross = XMVector3Cross(prevVector, arbitrary);
							cross = XMVector3Normalize(cross);
							angle = XM_PI;
						}
						else {
							// 축 정규화
							cross = XMVector3Normalize(cross);
						}

						// 쿼터니언 생성
						q = XMQuaternionRotationAxis(cross, angle);

						// 이전 벡터 업데이트
						prevVector = currentVector;
					}
				}
			}
			else if (m_rightButton)
			{
				static float prevRatio = 0.0f;
				static XMVECTOR prevPos{ 0.0f };
				if (m_dragStartFlag)
				{
					m_dragStartFlag = false;
					float nearToFar = XMVectorGetX(XMVector3Length(XMVectorSubtract(rayWorldFar, rayWorldNear)));
					prevRatio = dist / nearToFar;
					prevPos = pickVec;
				}
				else
				{
					XMVECTOR nearToFarVec = XMVectorSubtract(rayWorldFar, rayWorldNear);
					XMVECTOR newPos = rayWorldNear + nearToFarVec * prevRatio;
					float newPosDist = XMVectorGetX(XMVector3Length((XMVectorSubtract(newPos, prevPos))));
					if (newPosDist > 1e-3)
					{
						dragTranslation = XMVectorSubtract(newPos, prevPos);
						prevPos = newPos;
					}
				}
			}

			selectedModel->Update(q, dragTranslation);
		}
	}
}