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
	virtual void Destroy() override;

private:
	// Flags
	GuiState m_guiState;

	SamplingConstants m_combineConsts;

	// Models
	shared_ptr<Model> m_skybox;
	shared_ptr<Model> m_board;
	shared_ptr<Model> m_mirror;
	XMFLOAT4 m_mirrorPlane;
	float m_mirrorAlpha = 1.0f;
	float m_blendFactor[4] = { 0.5f, 0.5f, 0.5f, 1.0f };
	unordered_map<string, shared_ptr<Model>> m_models;
	shared_ptr<BoundingSphere> m_boundingSphere;
	shared_ptr<Model> m_cursorSphere;
	shared_ptr<Model> m_lightSphere[MAX_LIGHTS];

	shared_ptr<Model> m_screenSquare;
	shared_ptr<Model> m_boundsBox;

	ShapesInfo m_shapesInfo;

	enum GuiIndex {
		GENERAL,
		OBJECTS,
		SHAPES,
		LIGHT,
		ENV,
		FOG,
		POST_PROCESS,
		MIRROR,
		SCENE,
		MATERIAL,
		SPH,
	};

	struct DragState {
		bool isDragging = false;
		std::string draggedItem;
		ImTextureID draggedTexture;
		UINT albedoTextureIndex;
		ImVec2 dragOffset;
		bool isReleased;
	} dragState;

	// Synchronization
	static MainEngine* s_app;


private:
	void UpdateMouseControl();
	void UpdateLight(float dt);
	UINT DrawTableRow(const char* label, std::function<UINT()> uiElement);

private:
	void SphCalcPass();
	void SphRenderPass();

	void InitPreFrame();
	void CreateShapes();
	void DepthOnlyPass();
	void ScenePass();
	void ResolvePass();
	void PostEffectsPass();
	void PostProcessPass();
	void ImGUIPass();

};