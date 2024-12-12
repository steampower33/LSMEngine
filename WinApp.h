#pragma once

#include "windows.h"

namespace WindowApplication
{
	class WinApp
	{
	public:
		static int Run(HINSTANCE hInstance, int nShowCmd);

	private:
		// ������ ���ν��� ����
		static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	
	};
}