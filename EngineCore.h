#pragma once

#include "stdafx.h"
#include <iostream>

namespace EngineCore
{
	class AppBase
	{

	public:
		AppBase();
		~AppBase();

		
		bool RunApplication(HINSTANCE hInstance, int nShowCmd);
		
		// initializeWindow
		bool InitializeWindow(HINSTANCE hInstance, int ShowWnd, bool fullscreen);

		// callback function for windows messages
		LRESULT MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

		void mainloop();

	private:
		// Handle to the window
		HWND hwnd = NULL;

		// name of the window (not the title)
		LPCTSTR WindowName = L"LSMEngine";

		// title of the window
		LPCTSTR WindowTitle = L"LSMEngine Window";

		// width and height of the window
		int Width = 800;
		int Height = 600;

		// is window full screen?
		bool FullScreen = false;

		// we will exit the program when this becomes false
		bool Running = true;

		
	};
}