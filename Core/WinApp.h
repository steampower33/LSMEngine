#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#define NOMINMAX
#include <windows.h>
#include <windowsx.h>

#include <shlobj.h>
#include <strsafe.h>

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"

#include "EngineBase.h"

class EngineBase;

class WinApp
{
public:
	static int Run(EngineBase* pMainEngine, HINSTANCE hInstance, int nShowCmd);
	static HWND m_hwnd;

private:
	// 윈도우 프로시저 선언
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	static std::wstring GetLatestWinPixGpuCapturerPath();

};