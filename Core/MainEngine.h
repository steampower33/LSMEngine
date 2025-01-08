#pragma once

#include "EngineBase.h"

#include "Model.h"
#include "GeometryGenerator.h"
#include "TextureManager.h"

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

	Light m_lightFromGUI;
	int m_lightType = 0;

	// Models
	shared_ptr<Model> m_skybox;
	unordered_map<string, shared_ptr<Model>> m_models;


	// Texture
	TextureManager textureManager;
};