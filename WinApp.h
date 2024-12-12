#pragma once

#include "windows.h"

namespace WindowApplication
{
	class WinApp
	{
	public:
		static int Run(HINSTANCE hInstance, int nShowCmd);

	private:
		// 윈도우 프로시저 선언
		static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	
	};
}