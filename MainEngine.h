#pragma once

#include "EngineBase.h"

namespace EngineCore
{
	class MainEngine : public EngineBase
	{
	public:
		MainEngine();

		virtual void Initialize() override;
		virtual void Update() override;
		virtual void Render() override;
		virtual void UpdateGUI() override;
	private:
		void UpdateSceneViewer();
		void RenderScene();
	};
}