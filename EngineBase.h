#pragma once

#include <string>
#include <iostream>

#include <shlobj.h>
#include <strsafe.h>

namespace EngineCore
{
	class EngineBase
	{
	public:
		EngineBase();
		virtual ~EngineBase();

		virtual void OnInit();
		
		static std::wstring GetLatestWinPixGpuCapturerPath();

	private:
		void LoadPipeline();
	};
}