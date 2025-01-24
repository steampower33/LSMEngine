#pragma once

#include "EngineBase.h"

#include "Model.h"
#include "GeometryGenerator.h"
#include "PostProcess.h"
#include "Ray.h"
#include <DirectXCollision.h>
#include <cmath>

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
	DirtyFlag dirtyFlag;

	Light m_lightFromGUI;
	int m_lightType = 0;

	SamplingConstants m_combineConsts;

	// Models
	shared_ptr<Model> m_skybox;
	shared_ptr<Model> m_board;
	shared_ptr<Model> m_mirror;
	float m_mirrorAlpha = 0.5f;
	float m_blendFactor[4] = { 0.5f, 0.5f, 0.5f, 1.0f };
	XMFLOAT4 m_mirrorPlane;
	unordered_map<string, shared_ptr<Model>> m_models;
	shared_ptr<BoundingSphere> m_boundingSphere;
	shared_ptr<Model> m_cursorSphere;
	shared_ptr<Model> m_lightSphere;

	shared_ptr<Model> m_screenSquare;

	// PostProcess
	shared_ptr<PostProcess> m_postProcess[FrameCount];


private:
	void UpdateMouseControl(XMMATRIX& view, XMMATRIX& proj);
};