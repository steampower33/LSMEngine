#pragma once

#include "AppBase.h"

namespace EngineCore
{
	class RenderApp : public AppBase
	{
	public:
		RenderApp();

		virtual bool Initialize() override;
		virtual void UpdateGUI() override;
		virtual void Update() override;
		virtual void Render() override;
	private:

	};
}