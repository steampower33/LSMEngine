#include "MainEngine.h"

MainEngine* MainEngine::s_app = nullptr;

MainEngine::MainEngine() : EngineBase()
{
	s_app = this;
}

void MainEngine::Initialize()
{
	LoadPipeline();
	LoadGUI();
	LoadContexts();

	m_pCurrFR = m_frameResources[m_frameIndex].get();

	ThrowIfFailed(m_pCurrFR->m_commandAllocator[0]->Reset());
	ThrowIfFailed(m_pCurrFR->m_commandList[0]->Reset(m_pCurrFR->m_commandAllocator[0].Get(), nullptr));

	m_textureManager->Initialize(m_device, m_pCurrFR->m_commandList[0]);
	for (UINT i = 0; i < FrameCount; i++)
		m_frameResources[i]->InitializeDescriptorHeaps(m_device, m_textureManager);

	{
		MeshData skybox = GeometryGenerator::MakeBox(50.0f);
		std::reverse(skybox.indices.begin(), skybox.indices.end());

		skybox.cubeEnvFilename = "./Assets/IBL/IBLEnvHDR.dds";
		skybox.cubeDiffuseFilename = "./Assets/IBL/IBLDiffuseHDR.dds";
		skybox.cubeSpecularFilename = "./Assets/IBL/IBLSpecularHDR.dds";
		skybox.cubeBrdfFilename = "./Assets/IBL/IBLBrdf.dds";

		m_skybox = make_shared<Model>(
			m_device, m_pCurrFR->m_commandList[0], m_commandQueue,
			vector{ skybox }, m_cubemapIndexConstsData, m_textureManager, XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f));
	}

	{
		MeshData meshData = GeometryGenerator::MakeSphere(0.025f, 20, 20);
		m_cursorSphere = make_shared<Model>(
			m_device, m_pCurrFR->m_commandList[0], m_commandQueue,
			vector{ meshData }, m_cubemapIndexConstsData, m_textureManager, XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f));
		m_cursorSphere->m_meshConstsBufferData.albedoFactor = XMFLOAT3(1.0f, 1.0f, 1.0);
		m_cursorSphere->m_meshConstsBufferData.useAlbedoMap = false;
	}

	{
		MeshData meshData = GeometryGenerator::MakeSquare(10.0f);

		XMFLOAT4 posMirror = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
		m_mirror = make_shared<Model>(
			m_device, m_pCurrFR->m_commandList[0], m_commandQueue,
			vector{ meshData }, m_cubemapIndexConstsData, m_textureManager, posMirror);
		m_mirror->m_meshConstsBufferData.albedoFactor = XMFLOAT3(0.1f, 0.1f, 0.1f);
		m_mirror->m_meshConstsBufferData.emissionFactor = XMFLOAT3(0.0f, 0.0f, 0.0f);
		m_mirror->m_meshConstsBufferData.metallicFactor = 0.5f;
		m_mirror->m_meshConstsBufferData.roughnessFactor = 0.3f;
		m_mirror->m_key = "mirror";
		//m_models.insert({ m_mirror->m_key, m_mirror });

		float degrees = 90.0f;
		float radians = XMConvertToRadians(degrees); // DirectXMath 함수 사용
		XMVECTOR AxisX{ 1.0f, 0.0f, 0.0f, 0.0f };
		XMVECTOR quaternion = XMQuaternionRotationAxis(AxisX, radians);

		XMVECTOR translation = XMVectorAdd(XMLoadFloat4(&posMirror), { 0.0f, -1.0f, 0.0f, 0.0f });
		m_mirror->UpdateQuaternionAndTranslation(quaternion, translation);

		XMVECTOR planeNormal{ 0.0f, 1.0f, 0.0f, 0.0f };

		XMVECTOR plane = XMPlaneFromPointNormal(translation, planeNormal);
		XMStoreFloat4(&m_mirrorPlane, plane);
	}

	{
		float radius = 0.5f;
		MeshData meshData = GeometryGenerator::MakeSphere(radius, 100, 100, { 2.0f, 2.0f });
		meshData.albedoFilename = "./Assets/worn-painted-metal-ue/worn-painted-metal_albedo.png";
		meshData.normalFilename = "./Assets/worn-painted-metal-ue/worn-painted-metal_normal-dx.png";
		meshData.heightFilename = "./Assets/worn-painted-metal-ue/worn-painted-metal_height.png";
		meshData.aoFilename = "./Assets/worn-painted-metal-ue/worn-painted-metal_ao.png";
		meshData.metallicFilename = "./Assets/worn-painted-metal-ue/worn-painted-metal_metallic.png";
		meshData.roughnessFilename = "./Assets/worn-painted-metal-ue/worn-painted-metal_roughness.png";

		shared_ptr<Model> sphere = make_shared<Model>(
			m_device, m_pCurrFR->m_commandList[0], m_commandQueue,
			vector{ meshData }, m_cubemapIndexConstsData, m_textureManager, XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f));
		sphere->m_key = "sphere" + std::to_string(m_shapesInfo.sphereNum);
		m_models.insert({ sphere->m_key, sphere });
		m_shapesInfo.sphereNum++;
	}

	{
		float radius = 0.5f;
		MeshData meshData = GeometryGenerator::MakeSphere(radius, 100, 100, { 2.0f, 2.0f });
		meshData.albedoFilename = "./Assets/vented/vented-metal-panel1_albedo.png";
		meshData.normalFilename = "./Assets/vented/vented-metal-panel1_normal-ogl.png";
		meshData.heightFilename = "./Assets/vented/vented-metal-panel1_height.png";
		meshData.aoFilename = "./Assets/vented/vented-metal-panel1_ao.png";
		meshData.metallicFilename = "./Assets/vented/vented-metal-panel1_metallic.png";
		meshData.roughnessFilename = "./Assets/vented/vented-metal-panel1_roughness.png";

		shared_ptr<Model> sphere = make_shared<Model>(
			m_device, m_pCurrFR->m_commandList[0], m_commandQueue,
			vector{ meshData }, m_cubemapIndexConstsData, m_textureManager, XMFLOAT4(1.0f, 0.0f, 0.0f, 0.0f));
		sphere->m_key = "sphere" + std::to_string(m_shapesInfo.sphereNum);
		m_models.insert({ sphere->m_key, sphere });
		m_shapesInfo.sphereNum++;
	}

	{
		m_globalConstsData.light[0].radiance = XMFLOAT3{ 5.0f, 5.0f, 5.0f };
		m_globalConstsData.light[0].position = XMFLOAT3{ 0.0f, 2.0f, 0.0f };
		m_globalConstsData.light[0].direction = XMFLOAT3{ 0.0f, -1.0f, 0.0f };
		m_globalConstsData.light[0].spotPower = 10.0f;
		m_globalConstsData.light[0].radius = 0.02f;
		m_globalConstsData.light[0].type =
			LIGHT_SPOT | LIGHT_SHADOW; // Point with shadow;

		m_globalConstsData.light[1].radiance = XMFLOAT3{ 5.0f, 5.0f, 5.0f };
		m_globalConstsData.light[1].radius = 0.02f;
		m_globalConstsData.light[1].spotPower = 10.0f;
		m_globalConstsData.light[1].fallOffEnd = 20.0f;
		m_globalConstsData.light[1].type =
			LIGHT_SPOT | LIGHT_SHADOW; // Point with shadow;

		for (UINT i = 0; i < MAX_LIGHTS; i++)
		{
			MeshData meshData = GeometryGenerator::MakeSphere(1.0f, 20, 20);
			XMFLOAT4 spherePos{ m_globalConstsData.light[i].position.x, m_globalConstsData.light[i].position.y, m_globalConstsData.light[i].position.z, 0.0f };
			m_lightSphere[i] = make_shared<Model>(
				m_device, m_pCurrFR->m_commandList[0], m_commandQueue,
				vector{ meshData }, m_cubemapIndexConstsData, m_textureManager, spherePos);
			m_lightSphere[i]->m_meshConstsBufferData.albedoFactor = XMFLOAT3(1.0f, 1.0f, 0.0);
			m_lightSphere[i]->m_scale = m_globalConstsData.light[i].radius;
			m_lightSphere[i]->m_meshConstsBufferData.useAlbedoMap = false;
		}
	}

	// 후처리용 화면 사각형
	{
		MeshData meshData = GeometryGenerator::MakeSquare();
		m_screenSquare = make_shared<Model>(
			m_device, m_pCurrFR->m_commandList[0], m_commandQueue,
			vector{ meshData }, m_cubemapIndexConstsData, m_textureManager, XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f));
	}

	// 후처리
	for (int i = 0; i < FrameCount; i++)
		m_frameResources[i]->m_postProcess = make_shared<PostProcess>(
			m_device, m_pCurrFR->m_commandList[0], m_sceneSize.x, m_sceneSize.y, m_frameResources[i]->m_globalConstsData.fogSRVIndex);

	ThrowIfFailed(m_pCurrFR->m_commandList[0]->Close());

	ID3D12CommandList* ppCommandLists[] = { m_pCurrFR->m_commandList[0].Get() };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	m_pCurrFR->m_fenceValue++;

	if (multiFrame)
	{
		WaitForGpu();
	}
	else
	{
		WaitForPreviousFrame();
	}

}

void MainEngine::UpdateGUI()
{
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();

	ImGui::NewFrame();

	ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(m_width, m_height), ImGuiCond_Always);
	ImGui::Begin("Main Editor", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0)); // 부모 패널의 패딩
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 1.0f); // 모서리 둥글게

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.14f, 0.14f, 0.14f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.21f, 0.21f, 0.21f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.43f, 0.86f, 1.0f));

		ImVec2 availSize = ImGui::GetContentRegionAvail();

		ImGui::BeginChild("Scene Control", m_sceneControllerSize, true);

		// 왼쪽 패널의 너비 설정
		float leftPaneWidth = 60.0f;
		static int buttonIdx = 0;
		ImVec2 buttonSize(leftPaneWidth, 30); // 패널 너비와 동일하게 설정

		ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.0f); // 자식 패널의 테두리 크기

		// 왼쪽 패널
		{
			ImGui::BeginChild("LeftPane", ImVec2(leftPaneWidth, 0), true);

			// 버튼 추가
			if (ImGui::Button("General", buttonSize)) { buttonIdx = GENERAL; }
			if (ImGui::Button("Objects", buttonSize)) { buttonIdx = OBJECTS; }
			if (ImGui::Button("Shapes", buttonSize)) { buttonIdx = SHAPES; }
			if (ImGui::Button("Lights", buttonSize)) { buttonIdx = LIGHT; }
			if (ImGui::Button("Env", buttonSize)) { buttonIdx = ENV; }
			if (ImGui::Button("Fog", buttonSize)) { buttonIdx = FOG; }
			if (ImGui::Button("  Post\nProcess", buttonSize)) { buttonIdx = POST_PROCESS; }
			if (ImGui::Button("Mirror", buttonSize)) { buttonIdx = MIRROR; }

			ImGui::EndChild();
		}

		// 오른쪽 콘텐츠 영역
		{
			ImGui::SameLine();

			ImGui::BeginChild("RightPane", ImVec2(0, 0), true, ImGuiWindowFlags_None);
			if (buttonIdx == GENERAL)
			{
				ImGui::Text("Average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

				ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable;

				if (ImGui::BeginTable("GeneralTable", 2, flags))
				{
					DrawTableRow("Draw Normals", [&]() {
						return ImGui::Checkbox("##Draw Normals", &m_guiState.isDrawNormals);
						});

					DrawTableRow("WireFrame", [&]() {
						return ImGui::Checkbox("##WireFrame", &m_guiState.isWireframe);
						});

					ImGui::EndTable();
				}

			}
			else if (buttonIdx == OBJECTS)
			{
				for (const auto& model : m_models)
				{
					ImGui::PushID(model.first.c_str());

					if (ImGui::CollapsingHeader(model.first.c_str())) {
						ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable;

						if (ImGui::BeginTable("MirrorTable", 2, flags))
						{
							UINT flag = 0;

							flag += DrawTableRow("Metallic", [&]() {
								return ImGui::SliderFloat("##Metallic", &model.second.get()->m_meshConstsBufferData.metallicFactor, 0.0f, 1.0f);
								});

							flag += DrawTableRow("Roughness", [&]() {
								return ImGui::SliderFloat("##Roughness", &model.second.get()->m_meshConstsBufferData.roughnessFactor, 0.0f, 1.0f);
								});

							flag += DrawTableRow("Use AlbedoTexture", [&]() {
								return ImGui::Checkbox("##Use AlbedoTexture", &model.second.get()->m_useAlbedoMap);
								});

							flag += DrawTableRow("Use NormalMapping", [&]() {
								return ImGui::Checkbox("##Use NormalMapping", &model.second.get()->m_useNormalMap);
								});

							flag += DrawTableRow("Use HeightMapping", [&]() {
								return ImGui::Checkbox("##Use HeightMapping", &model.second.get()->m_useHeightMap);
								});

							flag += DrawTableRow("HeightScale", [&]() {
								return ImGui::SliderFloat("##HeightScale", &model.second.get()->m_meshConstsBufferData.heightScale, 0.0f, 0.1f);
								});

							flag += DrawTableRow("Use AO", [&]() {
								return ImGui::Checkbox("##Use AO", &model.second.get()->m_useAOMap);
								});

							flag += DrawTableRow("Use MetallicMap", [&]() {
								return ImGui::Checkbox("##Use MetallicMap", &model.second.get()->m_useMetallicMap);
								});

							flag += DrawTableRow("Use RoughnessMap", [&]() {
								return ImGui::Checkbox("##Use RoughnessMap", &model.second.get()->m_useRoughnessMap);
								});

							flag += DrawTableRow("Use EmissiveMap", [&]() {
								return ImGui::Checkbox("##Use EmissiveMap", &model.second.get()->m_useEmissiveMap);
								});

							flag += DrawTableRow("MeshLodBias", [&]() {
								return ImGui::SliderFloat("##MeshLodBias", &model.second.get()->m_meshConstsBufferData.meshLodBias, 0.0f, 10.0f);
								});

							if (flag)
							{
								m_guiState.isMeshChanged = true;
								m_guiState.changedMeshKey = model.first;
							}
							ImGui::EndTable();
						}
					}

					ImGui::PopID();
				}
			}
			else if (buttonIdx == SHAPES)
			{
				if (ImGui::Button("Sphere", buttonSize)) { m_shapesInfo.sphereCnt++; }
				ImGui::SameLine();
				if (ImGui::Button("Square", buttonSize)) { m_shapesInfo.squareCnt++; }
				ImGui::SameLine();
				if (ImGui::Button("Box", buttonSize)) { m_shapesInfo.boxCnt++; }
				if (ImGui::Button("Test", buttonSize))
				{
					m_shapesInfo.testAfterCnt++;
				}
			}
			else if (buttonIdx == LIGHT)
			{
				if (ImGui::CollapsingHeader("Light 1")) {
					ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable;

					if (ImGui::BeginTable("MirrorTable", 2, flags))
					{
						DrawTableRow("Radius", [&]() {
							return ImGui::SliderFloat("##Radius", &m_globalConstsData.light[0].radius, 0.0f, 0.2f);
							});
						m_lightSphere[0]->m_scale = m_globalConstsData.light[0].radius;
						ImGui::EndTable();
					}
				}

				if (ImGui::CollapsingHeader("Light 2")) {
					ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable;

					if (ImGui::BeginTable("MirrorTable", 2, flags))
					{
						DrawTableRow("Radius", [&]() {
							return ImGui::SliderFloat("##Radius", &m_globalConstsData.light[1].radius, 0.0f, 0.2f);
							});
						m_lightSphere[1]->m_scale = m_globalConstsData.light[1].radius;
						ImGui::EndTable();
					}
				}

				if (ImGui::CollapsingHeader("Point Light"))
				{
					ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable;

					if (ImGui::BeginTable("MirrorTable", 2, flags))
					{
						DrawTableRow("Rotate", [&]() {
							return ImGui::Checkbox("##Rotate", &m_lightRot);
							});
						ImGui::EndTable();
					}
				}
			}
			else if (buttonIdx == ENV)
			{
				ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable;

				if (ImGui::BeginTable("EnvTable", 2, flags))
				{
					UINT flag = 0;
					flag += DrawTableRow("Strength", [&]() {
						return ImGui::SliderFloat("##Strength", &m_globalConstsData.strengthIBL, 0.0f, 5.0f);
						});

					static const char* envOptions[] = { "Env", "Irradiance", "Specular" };

					flag += DrawTableRow("Options", [&]() {
						return ImGui::Combo("##Options", &m_globalConstsData.choiceEnvMap, envOptions, IM_ARRAYSIZE(envOptions));
						});

					flag += DrawTableRow("EnvLodBias", [&]() {
						return ImGui::SliderFloat("##EnvLodBias", &m_globalConstsData.envLodBias, 0.0f, 10.0f);
						});

					if (flag)
						m_guiState.isEnvChanged = true;
					ImGui::EndTable();
				}

			}
			else if (buttonIdx == FOG)
			{
				ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable;

				if (ImGui::BeginTable("FogTable", 2, flags))
				{
					UINT flag = 0;

					static const char* fogOptions[] = { "Render", "Depth" };
					flag += DrawTableRow("Options", [&]() {
						return ImGui::Combo("##Options", &m_globalConstsData.fogMode, fogOptions, IM_ARRAYSIZE(fogOptions));
						});

					flag += DrawTableRow("Depth Scale", [&]() {
						return ImGui::SliderFloat("##Depth Scale", &m_globalConstsData.depthScale, 0.0f, 1.0f);
						});

					flag += DrawTableRow("Fog Strength", [&]() {
						return ImGui::SliderFloat("##Fog Strength", &m_globalConstsData.fogStrength, 0.0f, 10.0f);
						});

					if (flag)
						m_dirtyFlag.postEffectsFlag = true;
					ImGui::EndTable();
				}
			}
			else if (buttonIdx == POST_PROCESS)
			{
				ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable;

				if (ImGui::BeginTable("PostProcessTable", 2, flags))
				{
					UINT flag = 0;

					flag += DrawTableRow("Strength", [&]() {
						return ImGui::SliderFloat("##Strength", &m_combineConsts.strength, 0.0f, 1.0f);
						});

					flag += DrawTableRow("Exposure", [&]() {
						return ImGui::SliderFloat("##Exposure", &m_combineConsts.exposure, 0.0f, 10.0f);
						});

					flag += DrawTableRow("Gamma", [&]() {
						return ImGui::SliderFloat("##Gamma", &m_combineConsts.gamma, 0.0f, 5.0f);
						});

					if (flag)
						m_dirtyFlag.postProcessFlag = true;
					ImGui::EndTable();
				}
			}
			else if (buttonIdx == MIRROR)
			{
				ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable;

				if (ImGui::BeginTable("MirrorTable", 2, flags))
				{
					UINT flag = 0;

					if (flag += DrawTableRow("Alpha", [&]() {
						return ImGui::SliderFloat("##Alpha", &m_mirrorAlpha, 0.0f, 1.0f);
						}))
					{
						m_blendFactor[0] = m_mirrorAlpha;
						m_blendFactor[1] = m_mirrorAlpha;
						m_blendFactor[2] = m_mirrorAlpha;
					}

					flag += DrawTableRow("Metallic", [&]() {
						return ImGui::SliderFloat("##Metallic", &m_mirror->m_meshConstsBufferData.metallicFactor, 0.0f, 1.0f);
						});

					flag += DrawTableRow("Roughness", [&]() {
						return ImGui::SliderFloat("##Roughness", &m_mirror->m_meshConstsBufferData.roughnessFactor, 0.0f, 1.0f);
						});

					if (flag)
					{
						m_guiState.isMirrorChanged = true;
					}

					ImGui::EndTable();
				}
			}
			ImGui::EndChild();
		}
		ImGui::PopStyleVar(3);
		ImGui::PopStyleColor(3);
		ImGui::EndChild();
	}

	// Scene
	{
		ImGui::SameLine();

		ImGui::BeginChild("Scene", m_sceneSize, true);

		ImVec2 availSize = ImGui::GetContentRegionAvail();

		CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle(m_textureManager->m_textureHeap->GetGPUDescriptorHandleForHeapStart(), m_cbvSrvSize* m_pCurrFR->m_sceneBufferIndex);
		ImGui::Image((ImTextureID)srvHandle.ptr, availSize);

		ImGui::EndChild();
	}

	// Assets Browser
	{

		//ImGui::SetNextWindowPos(m_assetsBrowserPos, ImGuiCond_Always);
		//ImGui::SetNextWindowSize(m_assetsBrowserSize, ImGuiCond_Always);
		ImGui::BeginChild("Assets Browser", m_assetsBrowserSize, true);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0)); // 부모 패널의 패딩

		const int numColumns = 10;  // 한 줄에 표시할 아이템 개수
		int itemIndex = 0;
		ImVec2 itemSize(120, 160); // 개별 아이템 크기
		ImVec2 imageSize(100, 100); // 이미지 크기
		float padding = (itemSize.x - imageSize.x) / 2.0f; // 좌우 여백

		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 1.0f); // 모서리 둥글게
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0)); // 부모 패널의 패딩
		for (const auto& pair : m_textureManager->m_textureInfos) // 텍스처 리스트 순회
		{
			if (itemIndex % numColumns != 0)
				ImGui::SameLine(); // 같은 줄에 배치

			ImGui::PushID(itemIndex); // 고유 ID 추가

			// 개별 아이템 박스
			ImGui::BeginChild("MaterialItem", ImVec2(120, 160), true, ImGuiWindowFlags_NoScrollbar);

			// 이미지 정렬 (상/하/좌/우 여백 통일)
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + padding); // 좌우 여백 맞추기
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10); // 위쪽 여백
			ImGui::Image((ImTextureID)pair.second.gpuHandle.ptr, imageSize);

			// 이미지 클릭 감지
			if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
			{
				dragState.isDragging = true;
				dragState.draggedItem = pair.second.filename;
				dragState.draggedTexture = (ImTextureID)pair.second.gpuHandle.ptr;
				ImVec2 mousePos = ImGui::GetMousePos();
				ImVec2 itemPos = ImGui::GetItemRectMin();
				dragState.dragOffset = ImVec2(mousePos.x - itemPos.x, mousePos.y - itemPos.y);
				dragState.albedoTextureIndex = pair.second.heapIndex;
			}

			// 구분선 정렬
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
			ImGui::Separator();

			// 텍스트 정렬
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + padding); // 텍스트 좌우 중앙 정렬
			ImGui::TextWrapped(pair.second.filename.c_str());

			ImGui::EndChild();
			ImGui::PopID(); // ID 제거

			itemIndex++;
		}
		ImGui::PopStyleVar(3);

		ImGui::EndChild();

		// 드래그 중이면 반투명한 미리보기 표시
		if (dragState.isDragging)
		{
			ImVec2 mousePos = ImGui::GetMousePos();
			ImVec2 drawPos = ImVec2(mousePos.x - dragState.dragOffset.x, mousePos.y - dragState.dragOffset.y);

			// 드래그 이미지를 항상 최상위에 렌더링
			ImGui::SetNextWindowFocus();
			ImGui::SetNextWindowPos(drawPos, ImGuiCond_Always);
			ImGui::SetNextWindowBgAlpha(0.5f); // 반투명 효과
			ImGui::Begin("DragPreview", nullptr,
				ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
				ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
				ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav |
				ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBackground);

			ImGui::Image(dragState.draggedTexture, ImVec2(100, 100));

			ImGui::End();

			// 마우스 놓으면 드래그 종료
			if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
			{
				dragState.isReleased = true;
				dragState.isDragging = false;
				dragState.draggedItem.clear();
			}
		}
	}

	ImGui::End();

	ImGui::Render();
}

void MainEngine::Update(float dt)
{
	m_camera->Update(m_mouseDeltaX, m_mouseDeltaY, dt, m_isMouseMove);

	UpdateMouseControl();
	UpdateLight(dt);

	if (m_guiState.isMeshChanged)
	{
		m_guiState.isMeshChanged = false;
		m_models[m_guiState.changedMeshKey]->UpdateState();
	}

	if (m_dirtyFlag.postEffectsFlag)
	{
		m_dirtyFlag.postEffectsFlag = false;

		for (UINT i = 0; i < FrameCount; i++)
		{
			m_frameResources[i]->m_globalConstsData.depthScale = m_globalConstsData.depthScale;
			m_frameResources[i]->m_globalConstsData.fogStrength = m_globalConstsData.fogStrength;
			m_frameResources[i]->m_globalConstsData.fogMode = m_globalConstsData.fogMode;
		}
	}

	if (m_dirtyFlag.postProcessFlag)
	{
		m_dirtyFlag.postProcessFlag = false;

		for (UINT i = 0; i < FrameCount; i++)
			m_frameResources[i]->m_postProcess->Update(m_combineConsts);
	}

	if (m_guiState.isMirrorChanged)
	{
		m_guiState.isMirrorChanged = false;
		m_mirror->OnlyCallConstsMemcpy();
	}

	m_pCurrFR->Update(m_camera, m_mirrorPlane, m_globalConstsData, m_shadowGlobalConstsData, m_cubemapIndexConstsData);
}

void MainEngine::LoadContexts()
{
	struct threadwrapper
	{
		static unsigned int WINAPI generateThunk(LPVOID lpParameter)
		{
			ThreadParameter* parameter = reinterpret_cast<ThreadParameter*>(lpParameter);
			s_app->CreateShapesThread();
			return 0;
		}

		static unsigned int WINAPI basicThunk(LPVOID lpParameter)
		{
			ThreadParameter* parameter = reinterpret_cast<ThreadParameter*>(lpParameter);
			s_app->WorkerThread(parameter->threadIndex);
			return 0;
		}

		static unsigned int WINAPI fogThunk(LPVOID lpParameter)
		{
			s_app->FogWorkerThread();
			return 0;
		}

		static unsigned int WINAPI postProcessThunk(LPVOID lpParameter)
		{
			s_app->PostProcessWorkerThread();
			return 0;
		}
	};

	m_createShapesWorkerThreadCount = 1;
	m_createShapesWorkerBegin = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_createShapesWorkerFinished = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_createShapesWorkerHandle = reinterpret_cast<HANDLE>(_beginthreadex(
		nullptr,
		0,
		threadwrapper::generateThunk,
		nullptr,
		0,
		nullptr));

	assert(m_createShapesWorkerBegin != NULL);
	assert(m_createShapesWorkerFinished != NULL);
	assert(m_createShapesWorkerHandle != NULL);

	for (int i = 0; i < NumContexts; i++)
	{
		m_workerBegin[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
		m_workerFinished[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
		m_threadParameters[i].threadIndex = i;
		m_threadHandles[i] = reinterpret_cast<HANDLE>(_beginthreadex(
			nullptr,
			0,
			threadwrapper::basicThunk,
			reinterpret_cast<LPVOID>(&m_threadParameters[i]),
			0,
			nullptr));

		assert(m_workerBegin[i] != NULL);
		assert(m_workerFinished[i] != NULL);
		assert(m_threadHandles[i] != NULL);
	}


	m_fogWorkerThreadCount = 1;
	m_fogWorkerBegin = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_fogWorkerFinished = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_fogThreadHandle = reinterpret_cast<HANDLE>(_beginthreadex(
		nullptr,
		0,
		threadwrapper::fogThunk,
		nullptr,
		0,
		nullptr));

	assert(m_fogWorkerBegin != NULL);
	assert(m_fogWorkerFinished != NULL);
	assert(m_fogThreadHandle != NULL);

	m_postProcessWorkerThreadCount = 1;
	m_postProcessWorkerBegin = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_postProcessWorkerFinished = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_postProcessThreadHandle = reinterpret_cast<HANDLE>(_beginthreadex(
		nullptr,
		0,
		threadwrapper::postProcessThunk,
		nullptr,
		0,
		nullptr));

	assert(m_postProcessWorkerBegin != NULL);
	assert(m_postProcessWorkerFinished != NULL);
	assert(m_postProcessThreadHandle != NULL);
}

void MainEngine::Render()
{
	SetEvent(m_createShapesWorkerBegin);
	for (int i = 0; i < NumContexts; i++)
	{
		SetEvent(m_workerBegin[i]); // Tell each worker to start drawing.
	}
	SetEvent(m_fogWorkerBegin);
	SetEvent(m_postProcessWorkerBegin);

	{
		WaitForSingleObject(m_createShapesWorkerFinished, INFINITE);
		ID3D12CommandList* cmds[1] = { m_pCurrFR->m_createShapesCmdList.Get() };
		m_commandQueue->ExecuteCommandLists(m_createShapesWorkerThreadCount, cmds);
	}

	{
		WaitForMultipleObjects(NumContexts, m_workerFinished, TRUE, INFINITE);
		ID3D12CommandList* cmds[NumContexts];
		for (UINT i = 0; i < NumContexts; i++)
			cmds[i] = m_pCurrFR->m_commandList[i].Get();
		m_commandQueue->ExecuteCommandLists(NumContexts, cmds);
		//SignalGPU();
	}

	{
		WaitForSingleObject(m_fogWorkerFinished, INFINITE);
		ID3D12CommandList* cmds[1] = { m_pCurrFR->m_fogCmdList.Get() };
		m_commandQueue->ExecuteCommandLists(m_fogWorkerThreadCount, cmds);
		//SignalGPU();
	}

	{
		WaitForSingleObject(m_postProcessWorkerFinished, INFINITE);
		ID3D12CommandList* cmds[1] = { m_pCurrFR->m_postProcessCmdList.Get() };
		m_commandQueue->ExecuteCommandLists(m_postProcessWorkerThreadCount, cmds);
		//SignalGPU();

	}

	// Present the frame.
	ThrowIfFailed(m_swapChain->Present(0, 0));

	if (multiFrame)
		MoveToNextFrame();
	else
		WaitForPreviousFrame();
}

void MainEngine::UpdateLight(float dt)
{
	static XMVECTOR axis{ 2.0f, 0.0f, 0.0f };

	if (m_lightRot)
	{
		XMMATRIX rotMat = XMMatrixRotationY(dt * 3.141592f * 0.5f);
		axis = XMVector3TransformCoord(axis, rotMat);
	}

	// 1번 LIGHT만 설정
	XMVECTOR focusPosition = XMVECTOR{ 0.0f, 0.0f, 0.0f };
	XMVECTOR posVec = XMVectorAdd(XMVECTOR{ 0.0f, 2.0f, 0.0f }, axis);
	XMStoreFloat3(&m_globalConstsData.light[1].position, posVec);
	XMVECTOR lightDirection = XMVector3Normalize(XMVectorSubtract(focusPosition, posVec));
	XMStoreFloat3(&m_globalConstsData.light[1].direction, lightDirection);
	XMStoreFloat4(&m_lightSphere[1]->m_position, posVec);

	for (UINT i = 0; i < MAX_LIGHTS; i++)
	{
		const auto& light = m_globalConstsData.light[i];
		if (light.type & LIGHT_SHADOW)
		{
			XMVECTOR up{ 0.0f, 1.0f, 0.0f };
			XMVECTOR lightPos = XMLoadFloat3(&light.position);
			XMVECTOR lightDir = XMLoadFloat3(&light.direction);

			if (abs(XMVectorGetX(XMVector3Dot(up, lightDir)) + 1.0f) < 1e-5)
				up = XMVECTOR{ 1.0f, 0.0f, 0.0f };

			// 그림자맵을 만들 때 필요
			XMMATRIX lightView = XMMatrixLookAtLH(
				lightPos, XMVectorAdd(lightPos, lightDir), up);

			XMMATRIX lightProj = XMMatrixPerspectiveFovLH(
				XMConvertToRadians(120.0f), 1.0f, 0.1f, 10.0f);

			m_shadowGlobalConstsData[i].eyeWorld = light.position;
			XMStoreFloat4x4(&m_shadowGlobalConstsData[i].view, XMMatrixTranspose(lightView));
			XMStoreFloat4x4(&m_shadowGlobalConstsData[i].proj, XMMatrixTranspose(lightProj));

			XMMATRIX lightViewProj = XMMatrixMultiply(lightView, lightProj);
			XMMATRIX lightViewProjTrans = XMMatrixTranspose(lightViewProj);
			XMStoreFloat4x4(&m_shadowGlobalConstsData[i].viewProj, lightViewProjTrans);

			XMVECTOR det;
			XMMATRIX lightInvProj;
			lightInvProj = XMMatrixInverse(&det, lightProj);
			if (XMVectorGetX(det) == 0.0f) {
				// 역행렬이 존재하지 않음
				assert(false && "역행렬이 존재하지 않습니다!");
			}
			XMMATRIX lightInvProjTrans = XMMatrixTranspose(lightInvProj);
			XMStoreFloat4x4(&m_shadowGlobalConstsData[i].invProj, lightInvProjTrans);

			XMStoreFloat4x4(&m_globalConstsData.light[i].viewProj, lightViewProjTrans);
			XMStoreFloat4x4(&m_globalConstsData.light[i].invProj, lightInvProjTrans);
		}
		m_lightSphere[i]->Update();
	}
}

void MainEngine::UpdateMouseControl()
{
	XMMATRIX view = m_camera->GetViewMatrix();
	XMMATRIX proj = m_camera->GetProjectionMatrix();

	XMVECTOR q = XMQuaternionIdentity();
	XMVECTOR dragTranslation{ 0.0f };

	if (m_leftButton || m_rightButton || dragState.isReleased)
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
		for (const auto& model : m_models)
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
			if (dragState.isReleased)
			{
				dragState.isReleased = false;
				for (const auto& mesh : selectedModel->m_meshes)
				{
					mesh->textureIndexConstsBufferData.albedoIndex = dragState.albedoTextureIndex;
					memcpy(mesh->textureIndexConstsBufferDataBegin, &mesh->textureIndexConstsBufferData, sizeof(mesh->textureIndexConstsBufferDataBegin));
				}
				cout << "Released" << endl;
			}
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

			selectedModel->UpdateQuaternionAndTranslation(q, dragTranslation);
		}
	}
}

void MainEngine::Destroy()
{

	{
		const UINT64 fenceValue = m_pCurrFR->m_fenceValue;
		const UINT64 lastCompletedFence = m_fence->GetCompletedValue();

		// Signal and increment the fence value.
		ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), m_pCurrFR->m_fenceValue));
		m_pCurrFR->m_fenceValue++;

		// Wait until the previous frame is finished.
		if (lastCompletedFence < fenceValue)
		{
			ThrowIfFailed(m_fence->SetEventOnCompletion(fenceValue, m_fenceEvent));
			WaitForSingleObject(m_fenceEvent, INFINITE);
		}
		CloseHandle(m_fenceEvent);
	}

	// Close thread events and thread handles.
	CloseHandle(m_createShapesWorkerBegin);
	CloseHandle(m_createShapesWorkerFinished);
	CloseHandle(m_createShapesWorkerHandle);
	for (int i = 0; i < NumContexts; i++)
	{
		CloseHandle(m_workerBegin[i]);
		CloseHandle(m_workerFinished[i]);
		CloseHandle(m_threadHandles[i]);
	}
	CloseHandle(m_fogWorkerBegin);
	CloseHandle(m_fogWorkerFinished);
	CloseHandle(m_fogThreadHandle);
	CloseHandle(m_postProcessWorkerBegin);
	CloseHandle(m_postProcessWorkerFinished);
	CloseHandle(m_postProcessThreadHandle);

	m_srvAlloc.Destroy();

	// Cleanup
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	// COM 해제
	CoUninitialize();
}

void MainEngine::CreateShapes()
{
	if (m_shapesInfo.testBeforeCnt < m_shapesInfo.testAfterCnt)
	{
		cout << m_shapesInfo.testAfterCnt << endl;

		float radius = 0.5f;
		MeshData meshData = GeometryGenerator::MakeSphere(radius, 100, 100, { 2.0f, 2.0f });

		XMFLOAT3 cameraPos = m_camera->GetEyePos();
		XMVECTOR posVec{ -5.0f, m_shapesInfo.testAfterCnt * 1.0f, 0.0f, 1.0f };
		for (UINT i = 0; i < 10; i++)
		{
			XMVECTOR offsetVec{ 1.0f, 0.0f, 0.0f, 0.0f };
			posVec = XMVectorAdd(posVec, offsetVec);
			XMFLOAT4 position;
			XMStoreFloat4(&position, posVec);

			shared_ptr<Model> model = make_shared<Model>(
				m_device, m_pCurrFR->m_createShapesCmdList, m_commandQueue,
				vector{ meshData }, m_cubemapIndexConstsData, m_textureManager, position);
			model->m_key = "sphere" + std::to_string(m_shapesInfo.sphereNum);
			m_models.insert({ model->m_key, model });
			m_shapesInfo.sphereNum++;
		}

		m_shapesInfo.testBeforeCnt = m_shapesInfo.testAfterCnt;
	}

	if (m_shapesInfo.sphereCnt > 0)
	{
		m_shapesInfo.sphereCnt = 0;
		float radius = 0.5f;
		MeshData meshData = GeometryGenerator::MakeSphere(radius, 100, 100, { 2.0f, 2.0f });
		XMFLOAT3 cameraPos = m_camera->GetEyePos();
		XMFLOAT4 position{ cameraPos.x, cameraPos.y, cameraPos.z + 3.0f, 1.0f };

		shared_ptr<Model> model = make_shared<Model>(
			m_device, m_pCurrFR->m_createShapesCmdList, m_commandQueue,
			vector{ meshData }, m_cubemapIndexConstsData, m_textureManager, position);
		model->m_key = "sphere" + std::to_string(m_shapesInfo.sphereNum);
		m_models.insert({ model->m_key, model });
		m_shapesInfo.sphereNum++;
	}

	if (m_shapesInfo.squareCnt > 0)
	{
		m_shapesInfo.squareCnt = 0;

		MeshData meshData = GeometryGenerator::MakeSquare();
		XMFLOAT3 cameraPos = m_camera->GetEyePos();
		XMFLOAT4 position{ cameraPos.x, cameraPos.y, cameraPos.z + 3.0f, 1.0f };

		shared_ptr<Model> model = make_shared<Model>(
			m_device, m_pCurrFR->m_createShapesCmdList, m_commandQueue,
			vector{ meshData }, m_cubemapIndexConstsData, m_textureManager, position);
		model->m_key = "square" + std::to_string(m_shapesInfo.squareNum);
		m_models.insert({ model->m_key, model });
		m_shapesInfo.squareNum++;
	}

	if (m_shapesInfo.boxCnt > 0)
	{
		m_shapesInfo.boxCnt = 0;

		MeshData meshData = GeometryGenerator::MakeBox();
		XMFLOAT3 cameraPos = m_camera->GetEyePos();
		XMFLOAT4 position{ cameraPos.x, cameraPos.y, cameraPos.z + 3.0f, 1.0f };

		shared_ptr<Model> model = make_shared<Model>(
			m_device, m_pCurrFR->m_createShapesCmdList, m_commandQueue,
			vector{ meshData }, m_cubemapIndexConstsData, m_textureManager, position);
		model->m_key = "box" + std::to_string(m_shapesInfo.boxNum);
		m_models.insert({ model->m_key, model });
		m_shapesInfo.boxNum++;
	}
}

UINT MainEngine::DrawTableRow(const char* label, std::function<UINT()> uiElement)
{
	ImGui::TableNextRow();
	ImGui::TableSetColumnIndex(0);
	ImGui::Text("%s", label);
	ImGui::TableSetColumnIndex(1);
	return uiElement(); // UI 요소 실행 후 반환
}

void MainEngine::CreateShapesThread()
{
	while (true)
	{
		WaitForSingleObject(m_createShapesWorkerBegin, INFINITE);

		ThrowIfFailed(m_pCurrFR->m_createShapesCmdAlloc->Reset());
		ThrowIfFailed(m_pCurrFR->m_createShapesCmdList->Reset(m_pCurrFR->m_createShapesCmdAlloc.Get(), nullptr));

		CreateShapes();

		const float color[] = { 0.0f, 0.2f, 1.0f, 1.0f };
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pCurrFR->m_floatRTVHeap->GetCPUDescriptorHandleForHeapStart());
		CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_pCurrFR->m_floatDSVHeap->GetCPUDescriptorHandleForHeapStart());
		m_pCurrFR->m_createShapesCmdList->ClearRenderTargetView(rtvHandle, color, 0, nullptr);
		m_pCurrFR->m_createShapesCmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

		{
			m_pCurrFR->m_createShapesCmdList->SetGraphicsRootSignature(Graphics::rootSignature.Get());

			m_pCurrFR->m_createShapesCmdList->RSSetViewports(1, &m_sceneViewport);
			m_pCurrFR->m_createShapesCmdList->RSSetScissorRects(1, &m_sceneScissorRect);
			m_pCurrFR->m_createShapesCmdList->SetGraphicsRootConstantBufferView(0, m_pCurrFR->m_globalConstsUploadHeap.Get()->GetGPUVirtualAddress());

			ID3D12DescriptorHeap* ppHeaps[] = { m_textureManager->m_textureHeap.Get() };
			m_pCurrFR->m_createShapesCmdList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
			m_pCurrFR->m_createShapesCmdList->SetGraphicsRootConstantBufferView(3, m_pCurrFR->m_cubemapIndexConstsUploadHeap.Get()->GetGPUVirtualAddress());
			m_pCurrFR->m_createShapesCmdList->SetGraphicsRootDescriptorTable(5, m_textureManager->m_textureHeap->GetGPUDescriptorHandleForHeapStart());

			CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pCurrFR->m_floatRTVHeap->GetCPUDescriptorHandleForHeapStart());
			CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_pCurrFR->m_floatDSVHeap->GetCPUDescriptorHandleForHeapStart());
			m_pCurrFR->m_createShapesCmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

			if (m_guiState.isWireframe)
				m_pCurrFR->m_createShapesCmdList->SetPipelineState(Graphics::skyboxWirePSO.Get());
			else
				m_pCurrFR->m_createShapesCmdList->SetPipelineState(Graphics::skyboxSolidPSO.Get());
			m_skybox->RenderSkybox(m_device, m_pCurrFR->m_createShapesCmdList);
		}

		m_pCurrFR->m_createShapesCmdList->SetPipelineState(Graphics::basicSimplePSPSO.Get());

		for (UINT i = 0; i < MAX_LIGHTS; i++)
			m_lightSphere[i]->Render(m_device, m_pCurrFR->m_createShapesCmdList);

		if (m_selected && (m_leftButton || m_rightButton))
			m_cursorSphere->Render(m_device, m_pCurrFR->m_createShapesCmdList);

		m_pCurrFR->m_createShapesCmdList->Close();

		SetEvent(m_createShapesWorkerFinished);
	}
}

void MainEngine::WorkerThread(int threadIndex)
{
	assert(threadIndex >= 0);
	assert(threadIndex < NumContexts);

	while (threadIndex >= 0 && threadIndex < NumContexts)
	{
		WaitForSingleObject(m_workerBegin[threadIndex], INFINITE);

		std::vector<std::shared_ptr<Model>> modelList;
		{
			modelList.reserve(m_models.size());
			for (const auto& pair : m_models)
			{
				modelList.push_back(pair.second);
			}
		}

		{
			ThrowIfFailed(m_pCurrFR->m_commandAllocator[threadIndex]->Reset());
			ThrowIfFailed(m_pCurrFR->m_commandList[threadIndex]->Reset(m_pCurrFR->m_commandAllocator[threadIndex].Get(), nullptr));

			m_pCurrFR->m_commandList[threadIndex]->SetGraphicsRootSignature(Graphics::rootSignature.Get());

			ID3D12DescriptorHeap* ppHeaps[] = { m_textureManager->m_textureHeap.Get() };
			m_pCurrFR->m_commandList[threadIndex]->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
			m_pCurrFR->m_commandList[threadIndex]->SetGraphicsRootConstantBufferView(3, m_pCurrFR->m_cubemapIndexConstsUploadHeap.Get()->GetGPUVirtualAddress());
			m_pCurrFR->m_commandList[threadIndex]->SetGraphicsRootDescriptorTable(5, m_textureManager->m_textureHeap->GetGPUDescriptorHandleForHeapStart());
		}

		// ShadowDepthOnly
		{
			m_pCurrFR->m_commandList[threadIndex]->SetPipelineState(Graphics::shadowDepthOnlyPSO.Get());

			m_pCurrFR->m_commandList[threadIndex]->RSSetViewports(1, &m_pCurrFR->m_shadowViewport);
			m_pCurrFR->m_commandList[threadIndex]->RSSetScissorRects(1, &m_pCurrFR->m_shadowScissorRect);
			for (UINT i = 0; i < MAX_LIGHTS; i++)
			{
				CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_pCurrFR->m_shadowDepthOnlyDSVHeap->GetCPUDescriptorHandleForHeapStart(), i * m_dsvSize);
				m_pCurrFR->m_commandList[threadIndex]->OMSetRenderTargets(0, nullptr, FALSE, &dsvHandle);
				m_pCurrFR->m_commandList[threadIndex]->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
				m_pCurrFR->m_commandList[threadIndex]->SetGraphicsRootConstantBufferView(0, m_pCurrFR->m_shadowGlobalConstsUploadHeap[i].Get()->GetGPUVirtualAddress());

				for (size_t i = threadIndex; i < modelList.size(); i += NumContexts)
				{
					modelList[i]->Render(m_device, m_pCurrFR->m_commandList[threadIndex]);
				}
			}

			// shadowDepthOnlyBuffer State Change D3D12_RESOURCE_STATE_DEPTH_WRITE To D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			for (UINT i = 0; i < MAX_LIGHTS; i++)
				SetBarrier(m_pCurrFR->m_commandList[threadIndex], m_pCurrFR->m_shadowDepthOnlyDSBuffer[i],
					D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		}

		{
			m_pCurrFR->m_commandList[threadIndex]->RSSetViewports(1, &m_sceneViewport);
			m_pCurrFR->m_commandList[threadIndex]->RSSetScissorRects(1, &m_sceneScissorRect);

			m_pCurrFR->m_commandList[threadIndex]->SetGraphicsRootConstantBufferView(0, m_pCurrFR->m_globalConstsUploadHeap.Get()->GetGPUVirtualAddress());
		}

		{
			CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pCurrFR->m_floatRTVHeap->GetCPUDescriptorHandleForHeapStart());
			CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_pCurrFR->m_floatDSVHeap->GetCPUDescriptorHandleForHeapStart());
			m_pCurrFR->m_commandList[threadIndex]->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

			if (m_guiState.isWireframe)
				m_pCurrFR->m_commandList[threadIndex]->SetPipelineState(Graphics::basicWirePSO.Get());
			else
				m_pCurrFR->m_commandList[threadIndex]->SetPipelineState(Graphics::basicSolidPSO.Get());

			for (size_t i = threadIndex; i < modelList.size(); i += NumContexts)
			{
				modelList[i]->Render(m_device, m_pCurrFR->m_commandList[threadIndex]);
			}

			if (m_guiState.isDrawNormals)
			{
				for (size_t i = threadIndex; i < modelList.size(); i += NumContexts)
				{
					modelList[i]->RenderNormal(m_pCurrFR->m_commandList[threadIndex]);
				}
			}

			// Mirror
			m_pCurrFR->m_commandList[threadIndex]->SetPipelineState(Graphics::stencilMaskPSO.Get());
			m_pCurrFR->m_commandList[threadIndex]->OMSetStencilRef(1); // 참조 값 1로 설정
			m_mirror->Render(m_device, m_pCurrFR->m_commandList[threadIndex]);

			if (m_guiState.isWireframe)
				m_pCurrFR->m_commandList[threadIndex]->SetPipelineState(Graphics::reflectWirePSO.Get());
			else
				m_pCurrFR->m_commandList[threadIndex]->SetPipelineState(Graphics::reflectSolidPSO.Get());
			m_pCurrFR->m_commandList[threadIndex]->OMSetStencilRef(1); // 참조 값 1로 설정
			m_pCurrFR->m_commandList[threadIndex]->SetGraphicsRootConstantBufferView(0, m_pCurrFR->m_reflectConstsUploadHeap.Get()->GetGPUVirtualAddress());

			for (auto it = m_models.begin(); it != m_models.end(); ++it)
			{
				it->second->Render(m_device, m_pCurrFR->m_commandList[threadIndex]);
			}


			m_pCurrFR->m_commandList[threadIndex]->SetPipelineState(Graphics::mirrorBlendSolidPSO.Get());
			m_pCurrFR->m_commandList[threadIndex]->SetGraphicsRootConstantBufferView(0, m_pCurrFR->m_globalConstsUploadHeap.Get()->GetGPUVirtualAddress());
			m_pCurrFR->m_commandList[threadIndex]->OMSetBlendFactor(m_blendFactor);
			m_pCurrFR->m_commandList[threadIndex]->OMSetStencilRef(1); // 참조 값 1로 설정
			m_mirror->Render(m_device, m_pCurrFR->m_commandList[threadIndex]);
		}

		// shadowDepthOnlyBuffer State Change D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE To D3D12_RESOURCE_STATE_DEPTH_WRITE;
		for (UINT i = 0; i < MAX_LIGHTS; i++)
			SetBarrier(m_pCurrFR->m_commandList[threadIndex], m_pCurrFR->m_shadowDepthOnlyDSBuffer[i],
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);

		ThrowIfFailed(m_pCurrFR->m_commandList[threadIndex]->Close());

		SetEvent(m_workerFinished[threadIndex]);
	}
}

void MainEngine::FogWorkerThread()
{
	while (true)
	{
		WaitForSingleObject(m_fogWorkerBegin, INFINITE);

		ThrowIfFailed(m_pCurrFR->m_fogCmdAlloc->Reset());
		ThrowIfFailed(m_pCurrFR->m_fogCmdList->Reset(m_pCurrFR->m_fogCmdAlloc.Get(), nullptr));

		// Resolve
		{
			SetBarrier(m_pCurrFR->m_fogCmdList, m_pCurrFR->m_floatBuffers,
				D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_RESOLVE_SOURCE);

			m_pCurrFR->m_fogCmdList->ResolveSubresource(
				m_pCurrFR->m_resolvedBuffers.Get(),   // Resolve 대상 (단일 샘플 텍스처)
				0,                      // 대상 서브리소스 인덱스
				m_pCurrFR->m_floatBuffers.Get(),       // Resolve 소스 (MSAA 텍스처)
				0,                      // 소스 서브리소스 인덱스
				m_pCurrFR->m_floatBuffers->GetDesc().Format // Resolve 포맷
			);

			SetBarrier(m_pCurrFR->m_fogCmdList, m_pCurrFR->m_floatBuffers,
				D3D12_RESOURCE_STATE_RESOLVE_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

			SetBarrier(m_pCurrFR->m_fogCmdList, m_pCurrFR->m_resolvedBuffers,
				D3D12_RESOURCE_STATE_RESOLVE_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		}

		m_pCurrFR->m_fogCmdList->SetGraphicsRootSignature(Graphics::rootSignature.Get());

		m_pCurrFR->m_fogCmdList->RSSetViewports(1, &m_sceneViewport);
		m_pCurrFR->m_fogCmdList->RSSetScissorRects(1, &m_sceneScissorRect);

		ID3D12DescriptorHeap* ppHeaps[] = { m_textureManager->m_textureHeap.Get() };
		m_pCurrFR->m_fogCmdList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
		m_pCurrFR->m_fogCmdList->SetGraphicsRootConstantBufferView(0, m_pCurrFR->m_globalConstsUploadHeap.Get()->GetGPUVirtualAddress());
		m_pCurrFR->m_fogCmdList->SetGraphicsRootConstantBufferView(3, m_pCurrFR->m_cubemapIndexConstsUploadHeap.Get()->GetGPUVirtualAddress());
		m_pCurrFR->m_fogCmdList->SetGraphicsRootDescriptorTable(5, m_textureManager->m_textureHeap->GetGPUDescriptorHandleForHeapStart());

		// FogDepthOnly
		{
			CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_pCurrFR->m_depthOnlyDSVHeap->GetCPUDescriptorHandleForHeapStart());
			m_pCurrFR->m_fogCmdList->OMSetRenderTargets(0, nullptr, FALSE, &dsvHandle);
			m_pCurrFR->m_fogCmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

			m_pCurrFR->m_fogCmdList->SetPipelineState(Graphics::shadowDepthOnlyPSO.Get());
			for (const auto& model : m_models)
				model.second->Render(m_device, m_pCurrFR->m_fogCmdList);
			m_skybox->Render(m_device, m_pCurrFR->m_fogCmdList);
		}

		// fog
		{
			CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pCurrFR->m_fogRTVHeap->GetCPUDescriptorHandleForHeapStart());
			CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
			m_pCurrFR->m_fogCmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
			m_pCurrFR->m_fogCmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
			m_pCurrFR->m_fogCmdList->SetPipelineState(Graphics::postEffectsPSO.Get());

			SetBarrier(m_pCurrFR->m_fogCmdList, m_pCurrFR->m_depthOnlyDSBuffer,
				D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

			m_screenSquare->Render(m_device, m_pCurrFR->m_fogCmdList);

			SetBarrier(m_pCurrFR->m_fogCmdList, m_pCurrFR->m_depthOnlyDSBuffer,
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		}

		SetBarrier(m_pCurrFR->m_fogCmdList, m_pCurrFR->m_resolvedBuffers,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RESOLVE_DEST);

		ThrowIfFailed(m_pCurrFR->m_fogCmdList->Close());

		SetEvent(m_fogWorkerFinished);
	}
}

void MainEngine::PostProcessWorkerThread()
{
	while (true)
	{
		WaitForSingleObject(m_postProcessWorkerBegin, INFINITE);

		ThrowIfFailed(m_pCurrFR->m_postProcessCmdAlloc->Reset());
		ThrowIfFailed(m_pCurrFR->m_postProcessCmdList->Reset(m_pCurrFR->m_postProcessCmdAlloc.Get(), nullptr));

		SetBarrier(m_pCurrFR->m_postProcessCmdList, m_pCurrFR->m_fogBuffer,
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		SetBarrier(m_pCurrFR->m_postProcessCmdList, m_renderTargets[m_frameIndex],
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

		SetBarrier(m_pCurrFR->m_postProcessCmdList, m_pCurrFR->m_sceneBuffer,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

		{
			m_pCurrFR->m_postProcessCmdList->SetGraphicsRootSignature(Graphics::rootSignature.Get());
			m_pCurrFR->m_postProcessCmdList->SetPipelineState(Graphics::samplingPSO.Get());

			m_pCurrFR->m_postProcessCmdList->RSSetViewports(1, &m_sceneViewport);
			m_pCurrFR->m_postProcessCmdList->RSSetScissorRects(1, &m_sceneScissorRect);

			ID3D12DescriptorHeap* ppHeaps[] = { m_textureManager->m_textureHeap.Get() };
			m_pCurrFR->m_postProcessCmdList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
			m_pCurrFR->m_postProcessCmdList->SetGraphicsRootConstantBufferView(0, m_pCurrFR->m_globalConstsUploadHeap.Get()->GetGPUVirtualAddress());
			m_pCurrFR->m_postProcessCmdList->SetGraphicsRootDescriptorTable(5, m_textureManager->m_textureHeap->GetGPUDescriptorHandleForHeapStart());
		}

		// PostProcess
		{
			m_pCurrFR->m_postProcess->Render(m_device, m_pCurrFR->m_postProcessCmdList, m_pCurrFR->m_sceneBuffer,
				m_pCurrFR->m_sceneRTVHeap, m_textureManager->m_textureHeap, m_dsvHeap, m_frameIndex);
		}

		SetBarrier(m_pCurrFR->m_postProcessCmdList, m_pCurrFR->m_sceneBuffer,
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		SetBarrier(m_pCurrFR->m_postProcessCmdList, m_pCurrFR->m_fogBuffer,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

		m_pCurrFR->m_postProcessCmdList->RSSetViewports(1, &m_viewport);
		m_pCurrFR->m_postProcessCmdList->RSSetScissorRects(1, &m_scissorRect);

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_rtvSize * m_frameIndex);
		CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
		m_pCurrFR->m_postProcessCmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
		const float color[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		m_pCurrFR->m_postProcessCmdList->ClearRenderTargetView(rtvHandle, color, 0, nullptr);
		m_pCurrFR->m_postProcessCmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		ID3D12DescriptorHeap* ppHeaps[] = { m_textureManager->m_textureHeap.Get() };
		m_pCurrFR->m_postProcessCmdList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_pCurrFR->m_postProcessCmdList.Get());

		SetBarrier(m_pCurrFR->m_postProcessCmdList, m_renderTargets[m_frameIndex],
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

		ThrowIfFailed(m_pCurrFR->m_postProcessCmdList->Close());

		SetEvent(m_postProcessWorkerFinished);
	}
}