#pragma once

#include "EngineBase.h"

#include "Model.h"
#include "GeometryGenerator.h"
#include "TextureManager.h"
#include "PostProcess.h"
#include "Ray.h"
#include <DirectXCollision.h>

class MainEngine : public EngineBase
{
public:
	MainEngine();

	virtual void Initialize() override;
	virtual void Update(float dt) override;
	virtual void Render() override;
	virtual void UpdateGUI() override;

private:
	// Constants
	ComPtr<ID3D12Resource> m_globalConstsUploadHeap;
	GlobalConstants m_globalConstsBufferData = {};
	UINT8* m_globalConstsBufferDataBegin = nullptr;

	ComPtr<ID3D12Resource> m_cubemapIndexConstsUploadHeap;
	CubemapIndexConstants m_cubemapIndexConstsBufferData = {};
	UINT8* m_cubemapIndexConstsBufferDataBegin = nullptr;

	GuiState guiState;
	DirtyFlag dirtyFlag;

	Light m_lightFromGUI;
	int m_lightType = 0;

	float threshold = 0.0f;
	float strength = 0.5f;

	// Models
	shared_ptr<Model> m_skybox;
	unordered_map<string, shared_ptr<Model>> m_models;
	shared_ptr<BoundingSphere> m_boundingSphere;
	shared_ptr<Model> m_cursorSphere;

	// PostProcess
	shared_ptr<PostProcess> m_postProcess[FrameCount];

	// Texture
	shared_ptr<TextureManager> m_textureManager;

private:
	void UpdateMouseControl(
		XMMATRIX& view, XMMATRIX& proj, 
		XMVECTOR& q, XMVECTOR& dragTranslation);
};