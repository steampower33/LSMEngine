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

		virtual void KeyDown(UINT8 key) override;
		virtual void KeyUp(UINT8 key) override;
	private:
		void UpdateSceneViewer();
		void RenderScene();
	};
}