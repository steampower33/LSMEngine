#pragma once

#include "EngineBase.h"

#include "Model.h"
#include "GeometryGenerator.h"

class MainEngine : public EngineBase
{
public:
	MainEngine();

	virtual void Initialize() override;
	virtual void Update(float dt) override;
	virtual void Render() override;
	virtual void UpdateGUI() override;

private:
	ComPtr<ID3D12Resource> m_globalConstsUploadHeap;
	GlobalConstants m_globalConstsBufferData = {};
	UINT8* m_globalConstsBufferDataBegin = nullptr;

	ComPtr<ID3D12Resource> m_cubemapIndexConstsUploadHeap;
	CubemapIndexConstants m_cubemapIndexConstsBufferData = {};
	UINT8* m_cubemapIndexConstsBufferDataBegin = nullptr;

	GuiState guiState;

	shared_ptr<Model> m_skybox;

	shared_ptr<Model> m_cursorSphere;

	vector<shared_ptr<Model>> m_models;

	CD3DX12_CPU_DESCRIPTOR_HANDLE m_textureHandle;
	UINT m_totalTextureCnt;
	unordered_map<string, int> textureIdx;

	Light m_lightFromGUI;

	int m_lightType = 0;
};