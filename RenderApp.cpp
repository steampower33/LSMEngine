#include "RenderApp.h"

namespace EngineCore
{
	RenderApp::RenderApp() : AppBase() {}

	bool RenderApp::Initialize()
	{
		if (!AppBase::Initialize())
			return false;
	}

	void RenderApp::UpdateGUI()
	{

	}

	void RenderApp::Update()
	{
		AppBase::Update();
	}

	void RenderApp::Render()
	{

		AppBase::Render();
	}
}