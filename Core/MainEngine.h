#pragma once

#include "EngineBase.h"

#include "Model.h"
#include "GeometryGenerator.h"
#include "TextureManager.h"
#include "PostProcess.h"

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
	float strength = 1.0f;

	// Models
	shared_ptr<Model> m_skybox;
	unordered_map<string, shared_ptr<Model>> m_models;
	shared_ptr<PostProcess> m_postProcess[FrameCount];

	// Texture
	TextureManager textureManager;
};