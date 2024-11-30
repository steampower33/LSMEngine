#include "EngineCore.h"

namespace EngineCore
{
	using namespace std;

	AppBase* g_appBase = nullptr;

	LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
		return g_appBase->MsgProc(hWnd, msg, wParam, lParam);
	}

	AppBase::AppBase()
	{

	}

	AppBase::~AppBase()
	{
	}


	bool AppBase::RunApplication(HINSTANCE hInstance, int nShowCmd)
	{
		// create the window
		if (!InitializeWindow(hInstance, nShowCmd, FullScreen))
		{
			MessageBox(0, L"Window Initialization - Failed",
				L"Error", MB_OK);
			return 1;
		}

		// start the main loop
		mainloop();

		return 0;
	}


	// create and show the window
	bool AppBase::InitializeWindow(HINSTANCE hInstance, int ShowWnd, bool fullscreen)
	{
		if (fullscreen)
		{
			HMONITOR hmon = MonitorFromWindow(hwnd,
				MONITOR_DEFAULTTONEAREST);
			MONITORINFO mi = { sizeof(mi) };
			GetMonitorInfo(hmon, &mi);

			Width = mi.rcMonitor.right - mi.rcMonitor.left;
			Height = mi.rcMonitor.bottom - mi.rcMonitor.top;
		}

		WNDCLASSEX wc;

		wc.cbSize = sizeof(WNDCLASSEX);
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc = WndProc;
		wc.cbClsExtra = NULL;
		wc.cbWndExtra = NULL;
		wc.hInstance = hInstance;
		wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 2);
		wc.lpszMenuName = NULL;
		wc.lpszClassName = WindowName;
		wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

		if (!RegisterClassEx(&wc))
		{
			MessageBox(NULL, L"Error registering class",
				L"Error", MB_OK | MB_ICONERROR);
			return false;
		}

		hwnd = CreateWindowEx(NULL,
			WindowName,
			WindowTitle,
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT,
			Width, Height,
			NULL,
			NULL,
			hInstance,
			NULL);

		if (!hwnd)
		{
			MessageBox(NULL, L"Error creating window",
				L"Error", MB_OK | MB_ICONERROR);
			return false;
		}

		if (fullscreen)
		{
			SetWindowLong(hwnd, GWL_STYLE, 0);
		}

		ShowWindow(hwnd, ShowWnd);
		UpdateWindow(hwnd);

		return true;
	}

	void AppBase::mainloop() {
		MSG msg;
		ZeroMemory(&msg, sizeof(MSG));

		while (Running)
		{
			if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				if (msg.message == WM_QUIT)
					break;

				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else {
				
			}
		}
	}

	LRESULT AppBase::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg)
		{

		case WM_KEYDOWN:
			if (wParam == VK_ESCAPE) {
				if (MessageBox(0, L"Are you sure you want to exit?",
					L"Really?", MB_YESNO | MB_ICONQUESTION) == IDYES)
					DestroyWindow(hwnd);
			}
			return 0;

		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		}
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}

}