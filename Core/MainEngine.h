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

	GuiState guiState;

	shared_ptr<Model> m_skybox;

	vector<shared_ptr<Model>> m_models;

	CD3DX12_CPU_DESCRIPTOR_HANDLE m_textureHandle;
	UINT m_totalTextureCnt;
	unordered_map<string, int> textureIdx;
};