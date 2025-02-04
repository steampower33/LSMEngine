#include "MainEngine.h"

MainEngine::MainEngine() : EngineBase() 
{
}

void MainEngine::Initialize()
{
	LoadPipeline();
	m_pCurrFR = m_frameResources[m_frameIndex].get();

	// Important -> The command list is created and closed immediately. 
	// From here, you need to initialize the command list of m_pCurrFR and write here.
	ThrowIfFailed(m_pCurrFR->m_commandAllocator->Reset());
	ThrowIfFailed(m_pCurrFR->m_commandList->Reset(m_pCurrFR->m_commandAllocator.Get(), nullptr));

	// 1. m_textureManager Initialize
	// 2. m_frameResources Init DescriptorHeaps
	// All srv of render pass are also managed in m_textureHeap
	m_textureManager->Initialize(m_device, m_pCurrFR->m_commandList);
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
			m_device, m_pCurrFR->m_commandList, m_commandQueue,
			vector{ skybox }, m_cubemapIndexConstsData, m_textureManager, XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f));
	}

	{
		MeshData meshData = GeometryGenerator::MakeSphere(0.025f, 20, 20);
		m_cursorSphere = make_shared<Model>(
			m_device, m_pCurrFR->m_commandList, m_commandQueue,
			vector{ meshData }, m_cubemapIndexConstsData, m_textureManager, XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f));
		m_cursorSphere->m_meshConstsBufferData.albedoFactor = XMFLOAT3(1.0f, 1.0f, 1.0);
		m_cursorSphere->m_meshConstsBufferData.useAlbedoMap = false;
	}

	//{
	//	MeshData meshData = GeometryGenerator::MakeSquare(10.0f);

	//	m_board = make_shared<Model>(
	//		m_device, m_pCurrFR->m_commandList, m_commandQueue,
	//		vector{ meshData }, m_cubemapIndexConstsData, m_textureManager, XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f));

	//	float degrees = 90.0f;
	//	float radians = XMConvertToRadians(degrees); // DirectXMath 함수 사용
	//	XMVECTOR AxisX{ 1.0f, 0.0f, 0.0f, 0.0f };
	//	XMVECTOR quaternion = XMQuaternionRotationAxis(AxisX, radians);

	//	XMVECTOR translation{ 0.0f, -2.0f, 0.0f, 0.0f };
	//	m_board->UpdateQuaternionAndTranslation(quaternion, translation);
	//	m_board->m_key = "board";
	//	m_models.insert({ m_board->m_key, m_board });
	//}

	{
		MeshData meshData = GeometryGenerator::MakeSquare(10.0f);

		XMFLOAT4 posMirror = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
		m_mirror = make_shared<Model>(
			m_device, m_pCurrFR->m_commandList, m_commandQueue,
			vector{ meshData }, m_cubemapIndexConstsData, m_textureManager, posMirror);
		m_mirror->m_meshConstsBufferData.albedoFactor = XMFLOAT3(0.1f, 0.1f, 0.1f);
		m_mirror->m_meshConstsBufferData.emissionFactor = XMFLOAT3(0.0f, 0.0f, 0.0f);
		m_mirror->m_meshConstsBufferData.metallicFactor = 0.5f;
		m_mirror->m_meshConstsBufferData.roughnessFactor = 0.3f;
		m_mirror->m_key = "mirror";

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
		std::vector<MeshData> meshDatas = GeometryGenerator::ReadFromFile("./Assets/DamagedHelmet/", "DamagedHelmet.gltf");

		shared_ptr<Model> helmet = make_shared<Model>(
			m_device, m_pCurrFR->m_commandList, m_commandQueue,
			meshDatas, m_cubemapIndexConstsData, m_textureManager, XMFLOAT4(1.0f, 0.0f, 0.0f, 0.0f));
		helmet->m_key = "helmet";
		m_models.insert({ helmet->m_key, helmet });
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
			m_device, m_pCurrFR->m_commandList, m_commandQueue,
			vector{ meshData }, m_cubemapIndexConstsData, m_textureManager, XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f));
		sphere->m_key = "sphere";
		m_models.insert({ sphere->m_key, sphere });
	}

	{
		m_globalConstsData.light[0].radiance = XMFLOAT3{ 5.0f, 5.0f, 5.0f };
		m_globalConstsData.light[0].position = XMFLOAT3{ 0.0f, 2.0f, 0.0f };
		m_globalConstsData.light[0].direction = XMFLOAT3{ 0.0f, -1.0f, 0.0f };
		m_globalConstsData.light[0].spotPower = 3.0f;
		m_globalConstsData.light[0].radius = 0.02f;
		m_globalConstsData.light[0].type =
			LIGHT_SPOT | LIGHT_SHADOW; // Point with shadow;

		m_globalConstsData.light[1].radiance = XMFLOAT3{ 5.0f, 5.0f, 5.0f };
		m_globalConstsData.light[1].radius = 0.02f;
		m_globalConstsData.light[1].spotPower = 3.0f;
		m_globalConstsData.light[1].fallOffEnd = 20.0f;
		m_globalConstsData.light[1].type =
			LIGHT_SPOT | LIGHT_SHADOW; // Point with shadow;

		for (UINT i = 0; i < MAX_LIGHTS; i++)
		{
			MeshData meshData = GeometryGenerator::MakeSphere(1.0f, 20, 20);
			XMFLOAT4 spherePos{ m_globalConstsData.light[i].position.x, m_globalConstsData.light[i].position.y, m_globalConstsData.light[i].position.z, 0.0f };
			m_lightSphere[i] = make_shared<Model>(
				m_device, m_pCurrFR->m_commandList, m_commandQueue,
				vector{ meshData }, m_cubemapIndexConstsData, m_textureManager, spherePos);
			m_lightSphere[i]->m_meshConstsBufferData.albedoFactor = XMFLOAT3(1.0f, 1.0f, 0.0);
			m_lightSphere[i]->m_scale = m_globalConstsData.light[i].radius;
			m_lightSphere[i]->m_meshConstsBufferData.useAlbedoMap = false;
		}
	}

	/*{
		MeshData meshData = GeometryGenerator::MakeBox(1.0f);
		shared_ptr<Model> box = make_shared<Model>(
			m_device, m_pCurrFR->m_commandList, m_commandQueue,
			vector{ meshData }, m_cubemapIndexConstsData, m_textureManager, XMFLOAT4(2.0f, 0.0f, 0.0f, 0.0f));
		box->m_key = "box";
		m_models.insert({ box->m_key, box });
	}*/

	// 후처리용 화면 사각형
	{
		MeshData meshData = GeometryGenerator::MakeSquare();
		m_screenSquare = make_shared<Model>(
			m_device, m_pCurrFR->m_commandList, m_commandQueue,
			vector{ meshData }, m_cubemapIndexConstsData, m_textureManager, XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f));
	}

	// 후처리
	for (int i = 0; i < FrameCount; i++)
		m_frameResources[i]->m_postProcess = make_shared<PostProcess>(
			m_device, m_pCurrFR->m_commandList, m_sceneSize.x, m_sceneSize.y, m_frameResources[i]->m_globalConstsData.fogSRVIndex);

	ThrowIfFailed(m_pCurrFR->m_commandList->Close());

	ID3D12CommandList* ppCommandLists[] = { m_pCurrFR->m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	LoadGUI();

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

	{
		// 창의 위치 및 크기 고정
		ImGui::SetNextWindowPos(m_sceneControllerPos, ImGuiCond_Always);
		ImGui::SetNextWindowSize(m_sceneControllerSize, ImGuiCond_Always);

		ImVec2 availSize = ImGui::GetContentRegionAvail();

		ImGui::Begin("Scene Control", nullptr,
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoCollapse);

		ImGui::Text("Average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

		if (ImGui::TreeNode("General"))
		{
			ImGui::Checkbox("Draw Normals", &m_guiState.isDrawNormals);
			ImGui::Checkbox("Wireframe", &m_guiState.isWireframe);

			ImGui::TreePop();
		}

		if (ImGui::TreeNode("Env Map"))
		{
			ImGui::SliderFloat("Strength", &m_globalConstsData.strengthIBL, 0.0f, 5.0f);
			ImGui::RadioButton("Env", &m_globalConstsData.choiceEnvMap, 0);
			ImGui::SameLine();
			ImGui::RadioButton("Irradiance", &m_globalConstsData.choiceEnvMap, 1);
			ImGui::SameLine();
			ImGui::RadioButton("Specular", &m_globalConstsData.choiceEnvMap, 2);
			ImGui::SliderFloat("EnvLodBias", &m_globalConstsData.envLodBias, 0.0f, 10.0f);
			ImGui::TreePop();
		}

		if (ImGui::TreeNode("Fog"))
		{
			UINT flag = 0;
			flag += ImGui::RadioButton("Render", &m_globalConstsData.fogMode, 1);
			ImGui::SameLine();
			flag += ImGui::RadioButton("Depth", &m_globalConstsData.fogMode, 2);
			flag += ImGui::SliderFloat("Depth Scale", &m_globalConstsData.depthScale, 0.0f, 1.0f);
			flag += ImGui::SliderFloat("Fog Strength", &m_globalConstsData.fogStrength, 0.0f, 10.0f);
			if (flag)
				m_dirtyFlag.postEffectsFlag = true;
			ImGui::TreePop();
		}

		//ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		if (ImGui::TreeNode("Post Process"))
		{
			UINT flag = 0;
			flag += ImGui::SliderFloat("Strength", &m_combineConsts.strength, 0.0f, 1.0f);
			flag += ImGui::SliderFloat("Exposure", &m_combineConsts.exposure, 0.0f, 10.0f);
			flag += ImGui::SliderFloat("Gamma", &m_combineConsts.gamma, 0.0f, 5.0f);
			if (flag)
				m_dirtyFlag.postProcessFlag = true;
			ImGui::TreePop();
		}

		if (ImGui::TreeNode("Mirror"))
		{
			if (ImGui::SliderFloat("Alpha", &m_mirrorAlpha, 0.0f, 1.0f))
			{
				m_blendFactor[0] = m_mirrorAlpha;
				m_blendFactor[1] = m_mirrorAlpha;
				m_blendFactor[2] = m_mirrorAlpha;
			}

			ImGui::SliderFloat("Metallic",
				&m_mirror->m_meshConstsBufferData.metallicFactor, 0.0f, 1.0f);
			ImGui::SliderFloat("Roughness",
				&m_mirror->m_meshConstsBufferData.roughnessFactor, 0.0f, 1.0f);

			ImGui::TreePop();
		}

		if (ImGui::TreeNode("Light 1")) {
			ImGui::SliderFloat("Radius", &m_globalConstsData.light[0].radius, 0.0f, 0.2f);
			m_lightSphere[0]->m_scale = m_globalConstsData.light[0].radius;
			ImGui::TreePop();
		}

		if (ImGui::TreeNode("Light 2")) {
			ImGui::SliderFloat("Radius", &m_globalConstsData.light[1].radius, 0.0f, 0.2f);
			m_lightSphere[1]->m_scale = m_globalConstsData.light[1].radius;
			ImGui::TreePop();
		}

		if (ImGui::TreeNode("Point Light"))
		{
			ImGui::Checkbox("Rotate", &m_lightRot);
			ImGui::TreePop();
		}

		for (const auto& model : m_models)
		{
			ImGui::PushID(model.first.c_str());

			//ImGui::SetNextItemOpen(true, ImGuiCond_Once);
			if (ImGui::TreeNode(model.first.c_str())) {
				if (ImGui::SliderFloat("Metallic", &model.second.get()->m_meshConstsBufferData.metallicFactor, 0.0f, 1.0f) ||
					ImGui::SliderFloat("Roughness", &model.second.get()->m_meshConstsBufferData.roughnessFactor, 0.0f, 1.0f) ||
					ImGui::Checkbox("Use AlbedoTexture", &model.second.get()->m_useAlbedoMap) ||
					ImGui::Checkbox("Use NormalMapping", &model.second.get()->m_useNormalMap) ||
					ImGui::Checkbox("Use HeightMapping", &model.second.get()->m_useHeightMap) ||
					ImGui::SliderFloat("HeightScale", &model.second.get()->m_meshConstsBufferData.heightScale, 0.0f, 0.1f) ||
					ImGui::Checkbox("Use AO", &model.second.get()->m_useAOMap) ||
					ImGui::Checkbox("Use MetallicMap", &model.second.get()->m_useMetallicMap) ||
					ImGui::Checkbox("Use RoughnessMap", &model.second.get()->m_useRoughnessMap) ||
					ImGui::Checkbox("Use EmissiveMap", &model.second.get()->m_useEmissiveMap) ||
					ImGui::SliderFloat("MeshLodBias", &model.second.get()->m_meshConstsBufferData.meshLodBias, 0.0f, 10.0f)
					)
				{
					m_guiState.isMeshChanged = true;
					m_guiState.changedMeshKey = model.first;
				}
				ImGui::TreePop();
			}

			ImGui::PopID();
		}

		ImGui::End();
	}

	// Scene
	{
		// 창의 위치 및 크기 고정
		ImGui::SetNextWindowPos(m_scenePos, ImGuiCond_Always);
		ImGui::SetNextWindowSize(m_sceneSize, ImGuiCond_Always);
		ImGui::Begin("Scene", nullptr,
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoCollapse);

		ImVec2 availSize = ImGui::GetContentRegionAvail();

		CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle(m_textureManager->m_textureHeap->GetGPUDescriptorHandleForHeapStart(), m_cbvSrvSize* m_pCurrFR->m_sceneBufferIndex);
		ImGui::Image((ImTextureID)srvHandle.ptr, availSize);
		ImGui::End();
	}

	// Rendering
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

	m_mirror->OnlyCallConstsMemcpy();

	m_pCurrFR->Update(m_camera, m_mirrorPlane, m_globalConstsData, m_shadowGlobalConstsData, m_cubemapIndexConstsData);
}

void MainEngine::Render()
{
	{
		ThrowIfFailed(m_pCurrFR->m_commandAllocator->Reset());
		ThrowIfFailed(m_pCurrFR->m_commandList->Reset(m_pCurrFR->m_commandAllocator.Get(), nullptr));

		m_pCurrFR->m_commandList->SetGraphicsRootSignature(Graphics::rootSignature.Get());

		ID3D12DescriptorHeap* ppHeaps[] = { m_textureManager->m_textureHeap.Get() };
		m_pCurrFR->m_commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
		m_pCurrFR->m_commandList->SetGraphicsRootConstantBufferView(3, m_pCurrFR->m_cubemapIndexConstsUploadHeap.Get()->GetGPUVirtualAddress());
		m_pCurrFR->m_commandList->SetGraphicsRootDescriptorTable(5, m_textureManager->m_textureHeap->GetGPUDescriptorHandleForHeapStart());
	}

	// ShadowDepthOnly
	{
		m_pCurrFR->m_commandList->SetPipelineState(Graphics::shadowDepthOnlyPSO.Get());

		m_pCurrFR->m_commandList->RSSetViewports(1, &m_pCurrFR->m_shadowViewport);
		m_pCurrFR->m_commandList->RSSetScissorRects(1, &m_pCurrFR->m_shadowScissorRect);
		for (UINT i = 0; i < MAX_LIGHTS; i++)
		{
			CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_pCurrFR->m_shadowDepthOnlyDSVHeap->GetCPUDescriptorHandleForHeapStart(), i * m_dsvSize);
			m_pCurrFR->m_commandList->OMSetRenderTargets(0, nullptr, FALSE, &dsvHandle);
			m_pCurrFR->m_commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
			m_pCurrFR->m_commandList->SetGraphicsRootConstantBufferView(0, m_pCurrFR->m_shadowGlobalConstsUploadHeap[i].Get()->GetGPUVirtualAddress());

			for (const auto& model : m_models)
				model.second->Render(m_device, m_pCurrFR->m_commandList);
			m_mirror->Render(m_device, m_pCurrFR->m_commandList);
			m_skybox->Render(m_device, m_pCurrFR->m_commandList);
		}

		// shadowDepthOnlyBuffer State Change D3D12_RESOURCE_STATE_DEPTH_WRITE To D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		for (UINT i = 0; i < MAX_LIGHTS; i++)
			SetBarrier(m_pCurrFR->m_commandList, m_pCurrFR->m_shadowDepthOnlyDSBuffer[i],
				D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}

	{
		m_pCurrFR->m_commandList->RSSetViewports(1, &m_sceneViewport);
		m_pCurrFR->m_commandList->RSSetScissorRects(1, &m_sceneScissorRect);
		
		m_pCurrFR->m_commandList->SetGraphicsRootConstantBufferView(0, m_pCurrFR->m_globalConstsUploadHeap.Get()->GetGPUVirtualAddress());
	}

	// FogDepthOnly
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_pCurrFR->m_depthOnlyDSVHeap->GetCPUDescriptorHandleForHeapStart());
		m_pCurrFR->m_commandList->OMSetRenderTargets(0, nullptr, FALSE, &dsvHandle);
		m_pCurrFR->m_commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		for (const auto& model : m_models)
			model.second->Render(m_device, m_pCurrFR->m_commandList);
		m_mirror->Render(m_device, m_pCurrFR->m_commandList);
		m_skybox->Render(m_device, m_pCurrFR->m_commandList);
	}

	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pCurrFR->m_floatRTVHeap->GetCPUDescriptorHandleForHeapStart());
		CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_pCurrFR->m_floatDSVHeap->GetCPUDescriptorHandleForHeapStart());
		m_pCurrFR->m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

		const float color[] = { 0.0f, 0.2f, 1.0f, 1.0f };
		m_pCurrFR->m_commandList->ClearRenderTargetView(rtvHandle, color, 0, nullptr);
		m_pCurrFR->m_commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

		if (m_guiState.isWireframe)
			m_pCurrFR->m_commandList->SetPipelineState(Graphics::skyboxWirePSO.Get());
		else
			m_pCurrFR->m_commandList->SetPipelineState(Graphics::skyboxSolidPSO.Get());
		m_skybox->RenderSkybox(m_device, m_pCurrFR->m_commandList);

		if (m_guiState.isWireframe)
			m_pCurrFR->m_commandList->SetPipelineState(Graphics::basicWirePSO.Get());
		else
			m_pCurrFR->m_commandList->SetPipelineState(Graphics::basicSolidPSO.Get());
		for (const auto& model : m_models)
			model.second->Render(m_device, m_pCurrFR->m_commandList);

		m_pCurrFR->m_commandList->SetPipelineState(Graphics::basicSimplePSPSO.Get());

		for (UINT i = 0; i < MAX_LIGHTS; i++)
			m_lightSphere[i]->Render(m_device, m_pCurrFR->m_commandList);

		if (m_selected && (m_leftButton || m_rightButton))
			m_cursorSphere->Render(m_device, m_pCurrFR->m_commandList);

		if (m_guiState.isDrawNormals)
		{
			for (const auto& model : m_models)
				model.second->RenderNormal(m_pCurrFR->m_commandList);
		}

		// Mirror
		m_pCurrFR->m_commandList->SetPipelineState(Graphics::stencilMaskPSO.Get());
		m_pCurrFR->m_commandList->OMSetStencilRef(1); // 참조 값 1로 설정
		m_mirror->Render(m_device, m_pCurrFR->m_commandList);

		if (m_guiState.isWireframe)
			m_pCurrFR->m_commandList->SetPipelineState(Graphics::reflectWirePSO.Get());
		else
			m_pCurrFR->m_commandList->SetPipelineState(Graphics::reflectSolidPSO.Get());
		m_pCurrFR->m_commandList->OMSetStencilRef(1); // 참조 값 1로 설정
		m_pCurrFR->m_commandList->SetGraphicsRootConstantBufferView(0, m_pCurrFR->m_reflectConstsUploadHeap.Get()->GetGPUVirtualAddress());
		m_pCurrFR->m_commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
		for (const auto& model : m_models)
			model.second->Render(m_device, m_pCurrFR->m_commandList);

		if (m_guiState.isWireframe)
			m_pCurrFR->m_commandList->SetPipelineState(Graphics::skyboxReflectWirePSO.Get());
		else
			m_pCurrFR->m_commandList->SetPipelineState(Graphics::skyboxReflectSolidPSO.Get());
		m_pCurrFR->m_commandList->OMSetStencilRef(1); // 참조 값 1로 설정
		m_skybox->RenderSkybox(m_device, m_pCurrFR->m_commandList);

		m_pCurrFR->m_commandList->SetPipelineState(Graphics::mirrorBlendSolidPSO.Get());
		m_pCurrFR->m_commandList->SetGraphicsRootConstantBufferView(0, m_pCurrFR->m_globalConstsUploadHeap.Get()->GetGPUVirtualAddress());
		m_pCurrFR->m_commandList->OMSetBlendFactor(m_blendFactor);
		m_pCurrFR->m_commandList->OMSetStencilRef(1); // 참조 값 1로 설정
		m_mirror->Render(m_device, m_pCurrFR->m_commandList);
	}

	// shadowDepthOnlyBuffer State Change D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE To D3D12_RESOURCE_STATE_DEPTH_WRITE;
	for (UINT i = 0; i < MAX_LIGHTS; i++)
		SetBarrier(m_pCurrFR->m_commandList, m_pCurrFR->m_shadowDepthOnlyDSBuffer[i],
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);

	{
		SetBarrier(m_pCurrFR->m_commandList, m_pCurrFR->m_floatBuffers,
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_RESOLVE_SOURCE);

		m_pCurrFR->m_commandList->ResolveSubresource(
			m_pCurrFR->m_resolvedBuffers.Get(),   // Resolve 대상 (단일 샘플 텍스처)
			0,                      // 대상 서브리소스 인덱스
			m_pCurrFR->m_floatBuffers.Get(),       // Resolve 소스 (MSAA 텍스처)
			0,                      // 소스 서브리소스 인덱스
			m_pCurrFR->m_floatBuffers->GetDesc().Format // Resolve 포맷
		);

		SetBarrier(m_pCurrFR->m_commandList, m_pCurrFR->m_floatBuffers,
			D3D12_RESOURCE_STATE_RESOLVE_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

		SetBarrier(m_pCurrFR->m_commandList, m_pCurrFR->m_resolvedBuffers,
			D3D12_RESOURCE_STATE_RESOLVE_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}

	// PostEffects
	{
		// fog
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pCurrFR->m_fogRTVHeap->GetCPUDescriptorHandleForHeapStart());
		CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
		m_pCurrFR->m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
		m_pCurrFR->m_commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
		m_pCurrFR->m_commandList->SetPipelineState(Graphics::postEffectsPSO.Get());

		SetBarrier(m_pCurrFR->m_commandList, m_pCurrFR->m_depthOnlyDSBuffer,
			D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		m_screenSquare->Render(m_device, m_pCurrFR->m_commandList);

		SetBarrier(m_pCurrFR->m_commandList, m_pCurrFR->m_depthOnlyDSBuffer,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);

		SetBarrier(m_pCurrFR->m_commandList, m_pCurrFR->m_fogBuffer,
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}

	SetBarrier(m_pCurrFR->m_commandList, m_renderTargets[m_frameIndex],
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

	SetBarrier(m_pCurrFR->m_commandList, m_pCurrFR->m_sceneBuffer,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

	// PostProcess
	{
		m_pCurrFR->m_postProcess->Render(m_device, m_pCurrFR->m_commandList, m_pCurrFR->m_sceneBuffer,
			m_pCurrFR->m_sceneRTVHeap, m_textureManager->m_textureHeap, m_dsvHeap, m_frameIndex);
	}

	SetBarrier(m_pCurrFR->m_commandList, m_pCurrFR->m_sceneBuffer,
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	SetBarrier(m_pCurrFR->m_commandList, m_pCurrFR->m_fogBuffer,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

	SetBarrier(m_pCurrFR->m_commandList, m_pCurrFR->m_resolvedBuffers,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RESOLVE_DEST);

	m_pCurrFR->m_commandList->RSSetViewports(1, &m_viewport);
	m_pCurrFR->m_commandList->RSSetScissorRects(1, &m_scissorRect);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_rtvSize * m_frameIndex);
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
	m_pCurrFR->m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
	const float color[] = { 0.0f, 0.2f, 1.0f, 1.0f };
	m_pCurrFR->m_commandList->ClearRenderTargetView(rtvHandle, color, 0, nullptr);
	m_pCurrFR->m_commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	ID3D12DescriptorHeap* ppHeaps[] = { m_textureManager->m_textureHeap.Get() };
	m_pCurrFR->m_commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_pCurrFR->m_commandList.Get());

	SetBarrier(m_pCurrFR->m_commandList, m_renderTargets[m_frameIndex],
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

	ThrowIfFailed(m_pCurrFR->m_commandList->Close());

	// Execute the command list.
	ID3D12CommandList* ppCommandLists[] = { m_pCurrFR->m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

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

			selectedModel->UpdateQuaternionAndTranslation(q, dragTranslation);
		}
	}
}

void MainEngine::Destroy()
{
	if (multiFrame)
	{
		WaitForGpu();
	}
	else
	{
		WaitForPreviousFrame();
	}

	CloseHandle(m_fenceEvent);
	m_srvAlloc.Destroy();

	// Cleanup
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	// COM 해제
	CoUninitialize();
}