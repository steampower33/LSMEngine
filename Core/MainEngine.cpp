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

	m_pCurrFR = m_frameResources[m_frameIndex].get();

	ThrowIfFailed(m_pCurrFR->m_cmdAlloc->Reset());
	ThrowIfFailed(m_pCurrFR->m_cmdList->Reset(m_pCurrFR->m_cmdAlloc.Get(), nullptr));

	m_textureManager->Initialize(m_device, m_pCurrFR->m_cmdList);
	for (UINT i = 0; i < FrameCount; i++)
		m_frameResources[i]->InitializeDescriptorHeaps(m_device, m_textureManager);

	m_sphSimulator = make_shared<SphSimulator>();
	m_sphSimulator->Initialize(m_device, m_pCurrFR->m_cmdList, m_width, m_height);

	m_camera->m_pos.z = -max(m_sphSimulator->m_maxBoundsX, m_sphSimulator->m_maxBoundsY) * 0.75f;

	{
		MeshData meshData = GeometryGenerator::MakeBoundsBox();

		XMFLOAT4 pos = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
		m_boundsBox = make_shared<Model>(
			m_device, m_pCurrFR->m_cmdList, m_commandQueue,
			vector{ meshData }, m_cubemapIndexConstsData, m_textureManager, pos);
		m_boundsBox->m_key = "m_boundsBox";
	}

	//{
	//	MeshData skybox = GeometryGenerator::MakeBox(50.0f);
	//	std::reverse(skybox.indices.begin(), skybox.indices.end());

	//	skybox.cubeEnvFilename = "./Assets/IBL/IBLEnvHDR.dds";
	//	skybox.cubeDiffuseFilename = "./Assets/IBL/IBLDiffuseHDR.dds";
	//	skybox.cubeSpecularFilename = "./Assets/IBL/IBLSpecularHDR.dds";
	//	skybox.cubeBrdfFilename = "./Assets/IBL/IBLBrdf.dds";

	//	m_skybox = make_shared<Model>(
	//		m_device, m_pCurrFR->m_cmdList, m_commandQueue,
	//		vector{ skybox }, m_cubemapIndexConstsData, m_textureManager, XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f));
	//}

	//{
	//	MeshData meshData = GeometryGenerator::MakeSphere(0.025f, 20, 20);
	//	m_cursorSphere = make_shared<Model>(
	//		m_device, m_pCurrFR->m_cmdList, m_commandQueue,
	//		vector{ meshData }, m_cubemapIndexConstsData, m_textureManager, XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f));
	//	m_cursorSphere->m_meshConstsBufferData.albedoFactor = XMFLOAT3(1.0f, 1.0f, 1.0);
	//	m_cursorSphere->m_meshConstsBufferData.useAlbedoMap = false;
	//}

	//{
	//	MeshData meshData = GeometryGenerator::MakeSquare(10.0f);

	//	XMFLOAT4 posMirror = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	//	m_mirror = make_shared<Model>(
	//		m_device, m_pCurrFR->m_cmdList, m_commandQueue,
	//		vector{ meshData }, m_cubemapIndexConstsData, m_textureManager, posMirror);
	//	m_mirror->m_meshConstsBufferData.albedoFactor = XMFLOAT3(0.1f, 0.1f, 0.1f);
	//	m_mirror->m_meshConstsBufferData.emissionFactor = XMFLOAT3(0.0f, 0.0f, 0.0f);
	//	m_mirror->m_meshConstsBufferData.metallicFactor = 0.5f;
	//	m_mirror->m_meshConstsBufferData.roughnessFactor = 0.3f;
	//	m_mirror->m_key = "mirror";
	//	//m_models.insert({ m_mirror->m_key, m_mirror });

	//	float degrees = 90.0f;
	//	float radians = XMConvertToRadians(degrees); // DirectXMath 함수 사용
	//	XMVECTOR AxisX{ 1.0f, 0.0f, 0.0f, 0.0f };
	//	XMVECTOR quaternion = XMQuaternionRotationAxis(AxisX, radians);

	//	XMVECTOR translation = XMVectorAdd(XMLoadFloat4(&posMirror), { 0.0f, -1.0f, 0.0f, 0.0f });
	//	m_mirror->UpdateQuaternionAndTranslation(quaternion, translation);

	//	XMVECTOR planeNormal{ 0.0f, 1.0f, 0.0f, 0.0f };

	//	XMVECTOR plane = XMPlaneFromPointNormal(translation, planeNormal);
	//	XMStoreFloat4(&m_mirrorPlane, plane);
	//}

	//{
	//	float radius = 0.5f;
	//	MeshData meshData = GeometryGenerator::MakeSphere(radius, 100, 100, { 2.0f, 2.0f });
	//	meshData.albedoFilename = "./Assets/worn/worn-painted-metal_albedo.png";
	//	meshData.normalFilename = "./Assets/worn/worn-painted-metal_normal-dx.png";
	//	meshData.heightFilename = "./Assets/worn/worn-painted-metal_height.png";
	//	meshData.aoFilename = "./Assets/worn/worn-painted-metal_ao.png";
	//	meshData.metallicFilename = "./Assets/worn/worn-painted-metal_metallic.png";
	//	meshData.roughnessFilename = "./Assets/worn/worn-painted-metal_roughness.png";

	//	shared_ptr<Model> sphere = make_shared<Model>(
	//		m_device, m_pCurrFR->m_cmdList, m_commandQueue,
	//		vector{ meshData }, m_cubemapIndexConstsData, m_textureManager, XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f));
	//	sphere->m_key = "sphere" + std::to_string(m_shapesInfo.sphereNum);
	//	m_models.insert({ sphere->m_key, sphere });
	//	m_shapesInfo.sphereNum++;
	//}

	//{
	//	float radius = 0.5f;
	//	MeshData meshData = GeometryGenerator::MakeSphere(radius, 100, 100, { 2.0f, 2.0f });
	//	meshData.albedoFilename = "./Assets/vented/vented-metal-panel1_albedo.png";
	//	meshData.normalFilename = "./Assets/vented/vented-metal-panel1_normal-dx.png";
	//	meshData.heightFilename = "./Assets/vented/vented-metal-panel1_height.png";
	//	meshData.aoFilename = "./Assets/vented/vented-metal-panel1_ao.png";
	//	meshData.metallicFilename = "./Assets/vented/vented-metal-panel1_metallic.png";
	//	meshData.roughnessFilename = "./Assets/vented/vented-metal-panel1_roughness.png";

	//	shared_ptr<Model> sphere = make_shared<Model>(
	//		m_device, m_pCurrFR->m_cmdList, m_commandQueue,
	//		vector{ meshData }, m_cubemapIndexConstsData, m_textureManager, XMFLOAT4(1.0f, 0.0f, 0.0f, 0.0f));
	//	sphere->m_key = "sphere" + std::to_string(m_shapesInfo.sphereNum);
	//	m_models.insert({ sphere->m_key, sphere });
	//	m_shapesInfo.sphereNum++;
	//}

	//{
	//	m_globalConstsData.light[0].radiance = XMFLOAT3{ 5.0f, 5.0f, 5.0f };
	//	m_globalConstsData.light[0].position = XMFLOAT3{ 0.0f, 2.0f, 0.0f };
	//	m_globalConstsData.light[0].direction = XMFLOAT3{ 0.0f, -1.0f, 0.0f };
	//	m_globalConstsData.light[0].spotPower = 10.0f;
	//	m_globalConstsData.light[0].radius = 0.02f;
	//	m_globalConstsData.light[0].type =
	//		LIGHT_SPOT | LIGHT_SHADOW; // Point with shadow;

	//	m_globalConstsData.light[1].radiance = XMFLOAT3{ 5.0f, 5.0f, 5.0f };
	//	m_globalConstsData.light[1].radius = 0.02f;
	//	m_globalConstsData.light[1].spotPower = 10.0f;
	//	m_globalConstsData.light[1].fallOffEnd = 20.0f;
	//	m_globalConstsData.light[1].type =
	//		LIGHT_SPOT | LIGHT_SHADOW; // Point with shadow;

	//	for (UINT i = 0; i < MAX_LIGHTS; i++)
	//	{
	//		MeshData meshData = GeometryGenerator::MakeSphere(1.0f, 20, 20);
	//		XMFLOAT4 spherePos{ m_globalConstsData.light[i].position.x, m_globalConstsData.light[i].position.y, m_globalConstsData.light[i].position.z, 0.0f };
	//		m_lightSphere[i] = make_shared<Model>(
	//			m_device, m_pCurrFR->m_cmdList, m_commandQueue,
	//			vector{ meshData }, m_cubemapIndexConstsData, m_textureManager, spherePos);
	//		m_lightSphere[i]->m_meshConstsBufferData.albedoFactor = XMFLOAT3(1.0f, 1.0f, 0.0);
	//		float radius = m_globalConstsData.light[i].radius;
	//		m_lightSphere[i]->m_scale = XMFLOAT3(radius, radius, radius);
	//		m_lightSphere[i]->m_meshConstsBufferData.useAlbedoMap = false;
	//	}
	//}

	//// 후처리용 화면 사각형
	//{
	//	MeshData meshData = GeometryGenerator::MakeSquare();
	//	m_screenSquare = make_shared<Model>(
	//		m_device, m_pCurrFR->m_cmdList, m_commandQueue,
	//		vector{ meshData }, m_cubemapIndexConstsData, m_textureManager, XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f));
	//}

	//// 후처리
	//for (int i = 0; i < FrameCount; i++)
	//	m_frameResources[i]->m_postProcess = make_shared<PostProcess>(
	//		m_device, m_pCurrFR->m_cmdList, m_sceneSize.x, m_sceneSize.y, m_frameResources[i]->m_globalConstsData.fogSRVIndex, m_frameResources[i]->m_globalConstsData.resolvedSRVIndex);

	ThrowIfFailed(m_pCurrFR->m_cmdList->Close());

	ID3D12CommandList* ppCommandLists[] = { m_pCurrFR->m_cmdList.Get() };
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

	ImGui::SetNextWindowSize(ImVec2(m_width, m_height), ImGuiCond_Always);
	ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
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
		static int buttonIdx = SPH;
		ImVec2 buttonSize(leftPaneWidth, 30); // 패널 너비와 동일하게 설정

		ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.0f); // 자식 패널의 테두리 크기

		//// 왼쪽 패널
		//{
		//	ImGui::BeginChild("LeftPane", ImVec2(leftPaneWidth, 0), true);

		//	// 버튼 추가
		//	if (ImGui::Button("General", buttonSize)) { buttonIdx = GENERAL; }
		//	if (ImGui::Button("Objects", buttonSize)) { buttonIdx = OBJECTS; }
		//	if (ImGui::Button("Shapes", buttonSize)) { buttonIdx = SHAPES; }
		//	if (ImGui::Button("Lights", buttonSize)) { buttonIdx = LIGHT; }
		//	if (ImGui::Button("Env", buttonSize)) { buttonIdx = ENV; }
		//	if (ImGui::Button("Fog", buttonSize)) { buttonIdx = FOG; }
		//	if (ImGui::Button("  Post\nProcess", buttonSize)) { buttonIdx = POST_PROCESS; }
		//	if (ImGui::Button("Mirror", buttonSize)) { buttonIdx = MIRROR; }
		//	if (ImGui::Button("SPH", buttonSize)) { buttonIdx = SPH; }

		//	ImGui::EndChild();
		//}

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

							flag += DrawTableRow("Visible", [&]() {
								return ImGui::Checkbox("##Visible", &model.second.get()->isVisible);
								});

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
						float radius = m_globalConstsData.light[0].radius;
						m_lightSphere[0]->m_scale = XMFLOAT3(radius, radius, radius);
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
						float radius = m_globalConstsData.light[0].radius;
						m_lightSphere[1]->m_scale = XMFLOAT3(radius, radius, radius);
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

					flag += DrawTableRow("Enable", [&]() {
						return ImGui::Checkbox("##Enable", &m_skybox->isVisible);
						});

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

					flag += DrawTableRow("Enable", [&]() {
						return ImGui::Checkbox("##Enable", &m_guiState.isPostEffectsEnabled);
						});

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
						m_guiState.isPostEffectsChanged = true;
					ImGui::EndTable();
				}
			}
			else if (buttonIdx == POST_PROCESS)
			{
				ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable;

				if (ImGui::BeginTable("PostProcessTable", 2, flags))
				{
					UINT flag = 0;

					flag += DrawTableRow("Enable", [&]() {
						return ImGui::Checkbox("##Enable", &m_guiState.isPostProcessEnabled);
						});

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
						m_guiState.isPostProcessChanged = true;
					ImGui::EndTable();
				}
			}
			else if (buttonIdx == MIRROR)
			{
				ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable;

				if (ImGui::BeginTable("MirrorTable", 2, flags))
				{
					UINT flag = 0;

					flag += DrawTableRow("Enable", [&]() {
						return ImGui::Checkbox("##Enable", &m_guiState.isMirrorEnabled);
						});

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
			else if (buttonIdx == SPH)
			{
				ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable;

				if (ImGui::BeginTable("SphTable", 2, flags))
				{
					float minValue = 0.0f;
					float maxValue = 100.0f;
					float dragValue = 0.001f;

					UINT flag = 0;

					flag += DrawTableRow("Width", [&]() {
						return ImGui::DragFloat("##Width", &m_sphSimulator->m_maxBoundsX, dragValue, minValue, maxValue, "%.2f");
						});

					flag += DrawTableRow("Height", [&]() {
						return ImGui::DragFloat("##Height", &m_sphSimulator->m_maxBoundsY, dragValue, minValue, maxValue, "%.2f");
						});

					flag += DrawTableRow("Mass", [&]() {
						return ImGui::DragFloat("##Mass", &m_sphSimulator->m_constantBufferData.mass, dragValue, minValue, maxValue, "%.2f");
						});

					flag += DrawTableRow("PressureCoeff", [&]() {
						return ImGui::DragFloat("##PressureCoeff", &m_sphSimulator->m_constantBufferData.pressureCoeff, dragValue, minValue, maxValue, "%.2f");
						});

					flag += DrawTableRow("Density0", [&]() {
						return ImGui::DragFloat("##Density0", &m_sphSimulator->m_constantBufferData.density0, dragValue, minValue, maxValue, "%.2f");
						});

					flag += DrawTableRow("Viscosity", [&]() {
						return ImGui::DragFloat("##Viscosity", &m_sphSimulator->m_constantBufferData.viscosity, dragValue, minValue, maxValue, "%.2f");
						});

					flag += DrawTableRow("SmoothingRadius", [&]() {
						return ImGui::DragFloat("##SmoothingRadius", &m_sphSimulator->m_smoothingRadius, dragValue, minValue, maxValue, "%.2f");
						});

					flag += DrawTableRow("Gravity", [&]() {
						return ImGui::DragFloat("##Gravity", &m_sphSimulator->m_gravityCoeff, dragValue, minValue, maxValue, "%.2f");
						});

					flag += DrawTableRow("CollisionDamping", [&]() {
						return ImGui::DragFloat("##CollisionDamping", &m_sphSimulator->m_collisionDamping, dragValue, minValue, maxValue, "%.2f");
						});

					ImGui::EndTable();
				}

				ImGui::Text("Average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
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

	//// Assets Browser
	//{

	//	//ImGui::SetNextWindowPos(m_assetsBrowserPos, ImGuiCond_Always);
	//	//ImGui::SetNextWindowSize(m_assetsBrowserSize, ImGuiCond_Always);
	//	ImGui::BeginChild("Assets Browser", m_assetsBrowserSize, true);

	//	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0)); // 부모 패널의 패딩

	//	const int numColumns = 10;  // 한 줄에 표시할 아이템 개수
	//	int itemIndex = 0;
	//	ImVec2 itemSize(120, 160); // 개별 아이템 크기
	//	ImVec2 imageSize(100, 100); // 이미지 크기
	//	float padding = (itemSize.x - imageSize.x) / 2.0f; // 좌우 여백

	//	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 1.0f); // 모서리 둥글게
	//	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0)); // 부모 패널의 패딩
	//	for (const auto& pair : m_textureManager->m_textureInfos) // 텍스처 리스트 순회
	//	{
	//		if (itemIndex % numColumns != 0)
	//			ImGui::SameLine(); // 같은 줄에 배치

	//		ImGui::PushID(itemIndex); // 고유 ID 추가

	//		// 개별 아이템 박스
	//		ImGui::BeginChild("MaterialItem", ImVec2(120, 160), true, ImGuiWindowFlags_NoScrollbar);

	//		// 이미지 정렬 (상/하/좌/우 여백 통일)
	//		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + padding); // 좌우 여백 맞추기
	//		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10); // 위쪽 여백
	//		ImGui::Image((ImTextureID)pair.second.gpuHandle.ptr, imageSize);

	//		// 이미지 클릭 감지
	//		if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
	//		{
	//			dragState.isDragging = true;
	//			dragState.draggedItem = pair.second.filename;
	//			dragState.draggedTexture = (ImTextureID)pair.second.gpuHandle.ptr;
	//			ImVec2 mousePos = ImGui::GetMousePos();
	//			ImVec2 itemPos = ImGui::GetItemRectMin();
	//			dragState.dragOffset = ImVec2(mousePos.x - itemPos.x, mousePos.y - itemPos.y);
	//			dragState.albedoTextureIndex = pair.second.heapIndex;
	//		}

	//		// 구분선 정렬
	//		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
	//		ImGui::Separator();

	//		// 텍스트 정렬
	//		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + padding); // 텍스트 좌우 중앙 정렬
	//		ImGui::TextWrapped(pair.second.filename.c_str());

	//		ImGui::EndChild();
	//		ImGui::PopID(); // ID 제거

	//		itemIndex++;
	//	}
	//	ImGui::PopStyleVar(3);

	//	ImGui::EndChild();

	//	// 드래그 중이면 반투명한 미리보기 표시
	//	if (dragState.isDragging)
	//	{
	//		ImVec2 mousePos = ImGui::GetMousePos();
	//		ImVec2 drawPos = ImVec2(mousePos.x - dragState.dragOffset.x, mousePos.y - dragState.dragOffset.y);

	//		// 드래그 이미지를 항상 최상위에 렌더링
	//		ImGui::SetNextWindowFocus();
	//		ImGui::SetNextWindowPos(drawPos, ImGuiCond_Always);
	//		ImGui::SetNextWindowBgAlpha(0.5f); // 반투명 효과
	//		ImGui::Begin("DragPreview", nullptr,
	//			ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
	//			ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
	//			ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav |
	//			ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBackground);

	//		ImGui::Image(dragState.draggedTexture, ImVec2(100, 100));

	//		ImGui::End();

	//		// 마우스 놓으면 드래그 종료
	//		if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
	//		{
	//			dragState.isReleased = true;
	//			dragState.isDragging = false;
	//			dragState.draggedItem.clear();
	//		}
	//	}
	//}

	ImGui::End();

	ImGui::Render();
}

void MainEngine::Update(float dt)
{
	m_camera->Update(m_mouseDeltaX, m_mouseDeltaY, dt, m_isMouseMove);

	m_pCurrFR->UpdateGlobalConsts(m_camera, m_globalConstsData);

	if (!m_isPaused)
		m_sphSimulator->Update(dt, m_forceKey);

	// Update BoundsBox
	{
		XMVECTOR minB = XMLoadFloat3(&m_sphSimulator->m_constantBufferData.minBounds);
		XMVECTOR maxB = XMLoadFloat3(&m_sphSimulator->m_constantBufferData.maxBounds);

		XMVECTOR size = XMVectorScale(XMVectorSubtract(maxB, minB), 0.5f);
		XMVECTOR center = XMVectorScale(XMVectorAdd(minB, maxB), 0.5f);

		XMStoreFloat3(&m_boundsBox->m_scale, size);
		XMStoreFloat4(&m_boundsBox->m_position, center);
		m_boundsBox->Update();
	}

	//UpdateMouseControl();
	//UpdateLight(dt);

	//if (m_guiState.isMeshChanged)
	//{
	//	m_guiState.isMeshChanged = false;
	//	m_models[m_guiState.changedMeshKey]->UpdateState();
	//}

	//if (m_guiState.isPostEffectsEnabled && m_guiState.isPostEffectsChanged)
	//{
	//	m_guiState.isPostEffectsChanged = false;

	//	for (UINT i = 0; i < FrameCount; i++)
	//	{
	//		m_frameResources[i]->m_globalConstsData.depthScale = m_globalConstsData.depthScale;
	//		m_frameResources[i]->m_globalConstsData.fogStrength = m_globalConstsData.fogStrength;
	//		m_frameResources[i]->m_globalConstsData.fogMode = m_globalConstsData.fogMode;
	//	}
	//}

	//if (m_guiState.isPostProcessEnabled)
	//{

	//	for (UINT i = 0; i < FrameCount; i++)
	//		m_frameResources[i]->m_postProcess->m_copyFilter->m_postEffectsEnabled = m_guiState.isPostEffectsEnabled;

	//	if (m_guiState.isPostProcessChanged)
	//	{
	//		m_guiState.isPostProcessChanged = false;
	//		for (UINT i = 0; i < FrameCount; i++)
	//			m_frameResources[i]->m_postProcess->Update(m_combineConsts);
	//	}
	//}

	//m_globalConstsData.isEnvEnabled = m_skybox->isVisible ? 1 : 0;

	//if (m_guiState.isMirrorChanged)
	//{
	//	m_guiState.isMirrorChanged = false;
	//	m_mirror->OnlyCallConstsMemcpy();
	//}
}

void MainEngine::Render()
{
	ThrowIfFailed(m_pCurrFR->m_cmdAlloc->Reset());
	ThrowIfFailed(m_pCurrFR->m_cmdList->Reset(m_pCurrFR->m_cmdAlloc.Get(), nullptr));

	if (!m_isPaused)
		SphCalcPass();

	SphRenderPass();
	ImGUIPass();

	//InitPreFrame();
	//CreateShapes();
	//DepthOnlyPass();
	//ScenePass();
	//ResolvePass();
	//PostEffectsPass();
	//PostProcessPass();
	//ImGUIPass();

	ThrowIfFailed(m_pCurrFR->m_cmdList->Close());

	// Execute the command list.
	ID3D12CommandList* ppCommandLists[] = { m_pCurrFR->m_cmdList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// Present the frame.
	ThrowIfFailed(m_swapChain->Present(0, 0));

	if (multiFrame)
		MoveToNextFrame();
	else
		WaitForPreviousFrame();
}

void MainEngine::SphCalcPass()
{
	m_sphSimulator->Compute(m_pCurrFR->m_cmdList);
}

void MainEngine::SphRenderPass()
{
	SetBarrier(m_pCurrFR->m_cmdList, m_renderTargets[m_frameIndex],
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

	m_pCurrFR->m_cmdList->SetGraphicsRootSignature(Graphics::basicRootSignature.Get());

	ID3D12DescriptorHeap* ppHeaps[] = { m_textureManager->m_textureHeap.Get() };
	m_pCurrFR->m_cmdList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	m_pCurrFR->m_cmdList->RSSetViewports(1, &m_sceneViewport);
	m_pCurrFR->m_cmdList->RSSetScissorRects(1, &m_sceneScissorRect);

	// Sph Render
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pCurrFR->m_floatRTVHeap->GetCPUDescriptorHandleForHeapStart());
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_pCurrFR->m_floatDSVHeap->GetCPUDescriptorHandleForHeapStart());
	m_pCurrFR->m_cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

	const float color[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	m_pCurrFR->m_cmdList->ClearRenderTargetView(rtvHandle, color, 0, nullptr);
	m_pCurrFR->m_cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	m_pCurrFR->m_cmdList->SetGraphicsRootSignature(Graphics::sphRenderRootSignature.Get());
	m_pCurrFR->m_cmdList->SetPipelineState(Graphics::sphPSO.Get());
	m_sphSimulator->Render(m_pCurrFR->m_cmdList, m_pCurrFR->m_globalConstsUploadHeap);

	m_pCurrFR->m_cmdList->SetPipelineState(Graphics::boundsBoxPSO.Get());
	m_pCurrFR->m_cmdList->SetGraphicsRootConstantBufferView(1, m_pCurrFR->m_globalConstsUploadHeap->GetGPUVirtualAddress());
	m_boundsBox->RenderBoundsBox(m_device, m_pCurrFR->m_cmdList, m_pCurrFR->m_globalConstsUploadHeap);

	// Resolve
	SetBarrier(m_pCurrFR->m_cmdList, m_pCurrFR->m_floatBuffers,
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_RESOLVE_SOURCE);

	SetBarrier(m_pCurrFR->m_cmdList, m_pCurrFR->m_sceneBuffer,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RESOLVE_DEST);

	m_pCurrFR->m_cmdList->ResolveSubresource(
		m_pCurrFR->m_sceneBuffer.Get(),   // Resolve 대상 (단일 샘플 텍스처)
		0,                      // 대상 서브리소스 인덱스
		m_pCurrFR->m_floatBuffers.Get(),       // Resolve 소스 (MSAA 텍스처)
		0,                      // 소스 서브리소스 인덱스
		m_pCurrFR->m_floatBuffers->GetDesc().Format // Resolve 포맷
	);

	SetBarrier(m_pCurrFR->m_cmdList, m_pCurrFR->m_sceneBuffer,
		D3D12_RESOURCE_STATE_RESOLVE_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	SetBarrier(m_pCurrFR->m_cmdList, m_pCurrFR->m_floatBuffers,
		D3D12_RESOURCE_STATE_RESOLVE_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
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

	m_srvAlloc.Destroy();

	// Cleanup
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	// COM 해제
	CoUninitialize();
}

UINT MainEngine::DrawTableRow(const char* label, std::function<UINT()> uiElement)
{
	ImGui::TableNextRow();
	ImGui::TableSetColumnIndex(0);
	ImGui::Text("%s", label);
	ImGui::TableSetColumnIndex(1);
	return uiElement();
}

void MainEngine::InitPreFrame()
{
	m_pCurrFR->m_cmdList->SetGraphicsRootSignature(Graphics::basicRootSignature.Get());

	ID3D12DescriptorHeap* ppHeaps[] = { m_textureManager->m_textureHeap.Get() };
	m_pCurrFR->m_cmdList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
	m_pCurrFR->m_cmdList->SetGraphicsRootConstantBufferView(3, m_pCurrFR->m_cubemapIndexConstsUploadHeap.Get()->GetGPUVirtualAddress());
	m_pCurrFR->m_cmdList->SetGraphicsRootDescriptorTable(5, m_textureManager->m_textureHeap->GetGPUDescriptorHandleForHeapStart());
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
				m_device, m_pCurrFR->m_cmdList, m_commandQueue,
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
			m_device, m_pCurrFR->m_cmdList, m_commandQueue,
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
			m_device, m_pCurrFR->m_cmdList, m_commandQueue,
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
			m_device, m_pCurrFR->m_cmdList, m_commandQueue,
			vector{ meshData }, m_cubemapIndexConstsData, m_textureManager, position);
		model->m_key = "box" + std::to_string(m_shapesInfo.boxNum);
		m_models.insert({ model->m_key, model });
		m_shapesInfo.boxNum++;
	}
}

void MainEngine::DepthOnlyPass()
{
	m_pCurrFR->m_cmdList->SetPipelineState(Graphics::shadowDepthOnlyPSO.Get());

	// ShadowDepthOnly Pass
	{
		m_pCurrFR->m_cmdList->RSSetViewports(1, &m_pCurrFR->m_shadowViewport);
		m_pCurrFR->m_cmdList->RSSetScissorRects(1, &m_pCurrFR->m_shadowScissorRect);

		for (UINT i = 0; i < MAX_LIGHTS; i++)
		{
			CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_pCurrFR->m_shadowDepthOnlyDSVHeap->GetCPUDescriptorHandleForHeapStart(), i * m_dsvSize);
			m_pCurrFR->m_cmdList->OMSetRenderTargets(0, nullptr, FALSE, &dsvHandle);
			m_pCurrFR->m_cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
			m_pCurrFR->m_cmdList->SetGraphicsRootConstantBufferView(0, m_pCurrFR->m_shadowGlobalConstsUploadHeap[i].Get()->GetGPUVirtualAddress());

			for (const auto& model : m_models)
				if (model.second->isVisible)
					model.second->Render(m_device, m_pCurrFR->m_cmdList);

			if (m_guiState.isMirrorEnabled) m_mirror->Render(m_device, m_pCurrFR->m_cmdList);
			if (m_skybox->isVisible) m_skybox->Render(m_device, m_pCurrFR->m_cmdList);
		}

		// shadowDepthOnlyBuffer State Change D3D12_RESOURCE_STATE_DEPTH_WRITE To D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		for (UINT i = 0; i < MAX_LIGHTS; i++)
			SetBarrier(m_pCurrFR->m_cmdList, m_pCurrFR->m_shadowDepthOnlyDSBuffer[i],
				D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}

	// FogDepthOnly
	{
		m_pCurrFR->m_cmdList->RSSetViewports(1, &m_sceneViewport);
		m_pCurrFR->m_cmdList->RSSetScissorRects(1, &m_sceneScissorRect);
		m_pCurrFR->m_cmdList->SetGraphicsRootConstantBufferView(0, m_pCurrFR->m_globalConstsUploadHeap.Get()->GetGPUVirtualAddress());

		CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_pCurrFR->m_depthOnlyDSVHeap->GetCPUDescriptorHandleForHeapStart());
		m_pCurrFR->m_cmdList->OMSetRenderTargets(0, nullptr, FALSE, &dsvHandle);
		m_pCurrFR->m_cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		for (const auto& model : m_models)
			if (model.second->isVisible)
				model.second->Render(m_device, m_pCurrFR->m_cmdList);

		if (m_guiState.isMirrorEnabled) m_mirror->Render(m_device, m_pCurrFR->m_cmdList);
		if (m_skybox->isVisible) m_skybox->Render(m_device, m_pCurrFR->m_cmdList);
	}
}

void MainEngine::ScenePass()
{
	// FogDepthOnly에서의 viewport 그대로 사용
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pCurrFR->m_floatRTVHeap->GetCPUDescriptorHandleForHeapStart());
		CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_pCurrFR->m_floatDSVHeap->GetCPUDescriptorHandleForHeapStart());
		m_pCurrFR->m_cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

		const float color[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		m_pCurrFR->m_cmdList->ClearRenderTargetView(rtvHandle, color, 0, nullptr);
		m_pCurrFR->m_cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

		// SkyBox
		if (m_skybox->isVisible)
		{
			if (m_guiState.isWireframe)
				m_pCurrFR->m_cmdList->SetPipelineState(Graphics::skyboxWirePSO.Get());
			else
				m_pCurrFR->m_cmdList->SetPipelineState(Graphics::skyboxSolidPSO.Get());
			m_skybox->RenderSkybox(m_device, m_pCurrFR->m_cmdList);
		}

		// Models
		if (m_guiState.isWireframe)
			m_pCurrFR->m_cmdList->SetPipelineState(Graphics::basicWirePSO.Get());
		else
			m_pCurrFR->m_cmdList->SetPipelineState(Graphics::basicSolidPSO.Get());
		for (const auto& model : m_models)
			if (model.second->isVisible)
				model.second->Render(m_device, m_pCurrFR->m_cmdList);

		// lightSphere, cursorSphere
		m_pCurrFR->m_cmdList->SetPipelineState(Graphics::basicSimplePSPSO.Get());

		/*for (UINT i = 0; i < MAX_LIGHTS; i++)
			m_lightSphere[i]->Render(m_device, m_pCurrFR->m_cmdList);*/

		if (m_selected && (m_leftButton || m_rightButton))
			m_cursorSphere->Render(m_device, m_pCurrFR->m_cmdList);

		if (m_guiState.isDrawNormals)
		{
			for (const auto& model : m_models)
				if (model.second->isVisible)
					model.second->RenderNormal(m_pCurrFR->m_cmdList);
		}

		// Mirror
		if (m_guiState.isMirrorEnabled)
		{
			m_pCurrFR->m_cmdList->SetPipelineState(Graphics::stencilMaskPSO.Get());
			m_pCurrFR->m_cmdList->OMSetStencilRef(1); // 참조 값 1로 설정
			m_mirror->Render(m_device, m_pCurrFR->m_cmdList);

			if (m_guiState.isWireframe)
				m_pCurrFR->m_cmdList->SetPipelineState(Graphics::reflectWirePSO.Get());
			else
				m_pCurrFR->m_cmdList->SetPipelineState(Graphics::reflectSolidPSO.Get());
			m_pCurrFR->m_cmdList->OMSetStencilRef(1); // 참조 값 1로 설정
			m_pCurrFR->m_cmdList->SetGraphicsRootConstantBufferView(0, m_pCurrFR->m_reflectConstsUploadHeap.Get()->GetGPUVirtualAddress());
			m_pCurrFR->m_cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
			for (const auto& model : m_models)
				model.second->Render(m_device, m_pCurrFR->m_cmdList);

			if (m_guiState.isWireframe)
				m_pCurrFR->m_cmdList->SetPipelineState(Graphics::skyboxReflectWirePSO.Get());
			else
				m_pCurrFR->m_cmdList->SetPipelineState(Graphics::skyboxReflectSolidPSO.Get());
			m_pCurrFR->m_cmdList->OMSetStencilRef(1); // 참조 값 1로 설정
			m_skybox->RenderSkybox(m_device, m_pCurrFR->m_cmdList);

			m_pCurrFR->m_cmdList->SetPipelineState(Graphics::mirrorBlendSolidPSO.Get());
			m_pCurrFR->m_cmdList->SetGraphicsRootConstantBufferView(0, m_pCurrFR->m_globalConstsUploadHeap.Get()->GetGPUVirtualAddress());
			m_pCurrFR->m_cmdList->OMSetBlendFactor(m_blendFactor);
			m_pCurrFR->m_cmdList->OMSetStencilRef(1); // 참조 값 1로 설정
			m_mirror->Render(m_device, m_pCurrFR->m_cmdList);
		}
	}

	// shadowDepthOnlyBuffer State Change D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE To D3D12_RESOURCE_STATE_DEPTH_WRITE;
	for (UINT i = 0; i < MAX_LIGHTS; i++)
		SetBarrier(m_pCurrFR->m_cmdList, m_pCurrFR->m_shadowDepthOnlyDSBuffer[i],
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);
}

void MainEngine::ResolvePass()
{
	// Resolve
	SetBarrier(m_pCurrFR->m_cmdList, m_pCurrFR->m_floatBuffers,
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_RESOLVE_SOURCE);

	// PostEffects, PostProcess 둘다 False
	if (!m_guiState.isPostEffectsEnabled && !m_guiState.isPostProcessEnabled)
	{
		SetBarrier(m_pCurrFR->m_cmdList, m_pCurrFR->m_sceneBuffer,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RESOLVE_DEST);

		m_pCurrFR->m_cmdList->ResolveSubresource(
			m_pCurrFR->m_sceneBuffer.Get(),   // Resolve 대상 (단일 샘플 텍스처)
			0,                      // 대상 서브리소스 인덱스
			m_pCurrFR->m_floatBuffers.Get(),       // Resolve 소스 (MSAA 텍스처)
			0,                      // 소스 서브리소스 인덱스
			m_pCurrFR->m_floatBuffers->GetDesc().Format // Resolve 포맷
		);

		SetBarrier(m_pCurrFR->m_cmdList, m_pCurrFR->m_sceneBuffer,
			D3D12_RESOURCE_STATE_RESOLVE_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}
	else
	{
		m_pCurrFR->m_cmdList->ResolveSubresource(
			m_pCurrFR->m_resolvedBuffers.Get(),   // Resolve 대상 (단일 샘플 텍스처)
			0,                      // 대상 서브리소스 인덱스
			m_pCurrFR->m_floatBuffers.Get(),       // Resolve 소스 (MSAA 텍스처)
			0,                      // 소스 서브리소스 인덱스
			m_pCurrFR->m_floatBuffers->GetDesc().Format // Resolve 포맷
		);
	}

	SetBarrier(m_pCurrFR->m_cmdList, m_pCurrFR->m_floatBuffers,
		D3D12_RESOURCE_STATE_RESOLVE_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

	SetBarrier(m_pCurrFR->m_cmdList, m_pCurrFR->m_resolvedBuffers,
		D3D12_RESOURCE_STATE_RESOLVE_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void MainEngine::PostEffectsPass()
{
	// fog
	if (m_guiState.isPostEffectsEnabled)
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
		m_pCurrFR->m_cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
		m_pCurrFR->m_cmdList->SetPipelineState(Graphics::postEffectsPSO.Get());

		if (m_guiState.isPostProcessEnabled)
		{
			CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pCurrFR->m_fogRTVHeap->GetCPUDescriptorHandleForHeapStart());
			m_pCurrFR->m_cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

			SetBarrier(m_pCurrFR->m_cmdList, m_pCurrFR->m_depthOnlyDSBuffer,
				D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

			m_screenSquare->Render(m_device, m_pCurrFR->m_cmdList);

			SetBarrier(m_pCurrFR->m_cmdList, m_pCurrFR->m_depthOnlyDSBuffer,
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		}
		else
		{
			CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pCurrFR->m_sceneRTVHeap->GetCPUDescriptorHandleForHeapStart());
			m_pCurrFR->m_cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

			SetBarrier(m_pCurrFR->m_cmdList, m_pCurrFR->m_sceneBuffer,
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

			SetBarrier(m_pCurrFR->m_cmdList, m_pCurrFR->m_depthOnlyDSBuffer,
				D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

			m_screenSquare->Render(m_device, m_pCurrFR->m_cmdList);

			SetBarrier(m_pCurrFR->m_cmdList, m_pCurrFR->m_depthOnlyDSBuffer,
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);

			SetBarrier(m_pCurrFR->m_cmdList, m_pCurrFR->m_sceneBuffer,
				D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		}
	}
}

void MainEngine::PostProcessPass()
{
	SetBarrier(m_pCurrFR->m_cmdList, m_renderTargets[m_frameIndex],
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

	if (m_guiState.isPostProcessEnabled)
	{
		SetBarrier(m_pCurrFR->m_cmdList, m_pCurrFR->m_fogBuffer,
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		SetBarrier(m_pCurrFR->m_cmdList, m_pCurrFR->m_sceneBuffer,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

		// PostProcess
		{
			m_pCurrFR->m_postProcess->Render(m_device, m_pCurrFR->m_cmdList, m_pCurrFR->m_sceneBuffer,
				m_pCurrFR->m_sceneRTVHeap, m_textureManager->m_textureHeap, m_dsvHeap, m_frameIndex);
		}

		SetBarrier(m_pCurrFR->m_cmdList, m_pCurrFR->m_sceneBuffer,
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		SetBarrier(m_pCurrFR->m_cmdList, m_pCurrFR->m_fogBuffer,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
	}

	SetBarrier(m_pCurrFR->m_cmdList, m_pCurrFR->m_resolvedBuffers,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RESOLVE_DEST);

	m_pCurrFR->m_cmdList->RSSetViewports(1, &m_viewport);
	m_pCurrFR->m_cmdList->RSSetScissorRects(1, &m_scissorRect);
}

void MainEngine::ImGUIPass()
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_rtvSize * m_frameIndex);
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
	m_pCurrFR->m_cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
	const float color[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	m_pCurrFR->m_cmdList->ClearRenderTargetView(rtvHandle, color, 0, nullptr);
	m_pCurrFR->m_cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	ID3D12DescriptorHeap* ppHeaps[] = { m_textureManager->m_textureHeap.Get() };
	m_pCurrFR->m_cmdList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_pCurrFR->m_cmdList.Get());

	SetBarrier(m_pCurrFR->m_cmdList, m_renderTargets[m_frameIndex],
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
}