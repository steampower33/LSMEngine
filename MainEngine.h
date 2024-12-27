#pragma once

#include "EngineBase.h"
#include "Model.h"

namespace EngineCore
{
	using namespace Renderer;

	class MainEngine : public EngineBase
	{
	public:
		MainEngine();

		virtual void Initialize() override;
		virtual void Update(float dt) override;
		virtual void Render() override;
		virtual void UpdateGUI() override;

	private:
		void UpdateSceneViewer();
		void RenderScene();

		std::vector<Model> models;

		bool m_isMouseOverScene;

		float m_mouseDeltaX;
		float m_mouseDeltaY;
	};
}