#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers.
#endif

#include <windows.h>
#include "EngineBase.h"

using namespace EngineCore;

namespace WindowApplication
{
	class WinApp
	{
	public:
		static int Run(EngineCore::EngineBase* pEngineBase, HINSTANCE hInstance, int nShowCmd);
		static HWND m_hwnd;

	private:
		// 윈도우 프로시저 선언
		static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


	};
}