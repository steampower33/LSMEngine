#include "WinApp.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace WindowApplication
{
	using namespace std;

	HWND WinApp::m_hwnd = nullptr;

	int WinApp::Run(EngineCore::EngineBase* pEngine, HINSTANCE hInstance, int nShowCmd)
	{
		// Check to see if a copy of WinPixGpuCapturer.dll has already been injected into the application.
		// This may happen if the application is launched through the PIX UI.
		if (GetModuleHandle(L"WinPixGpuCapturer.dll") == 0)
		{
			LoadLibrary(GetLatestWinPixGpuCapturerPath().c_str());
		}

		WNDCLASSEX wc = { 
			sizeof(wc), 
			CS_CLASSDC, 
			WndProc, 
			0L, 0L, 
			GetModuleHandle(NULL), NULL, NULL, NULL, NULL, L"LSMEngine", NULL };

		if (!RegisterClassEx(&wc)) return false;

		RECT windowRect = { 0, 0, pEngine->m_width, pEngine->m_height };
		AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, false);

		m_hwnd = CreateWindow(wc.lpszClassName, L"LSMEngineWindow",
			WS_OVERLAPPEDWINDOW,
			100, // 윈도우 좌측 상단의 x 좌표
			100, // 윈도우 좌측 상단의 y 좌표
			windowRect.right - windowRect.left, // 윈도우 가로 방향 해상도
			windowRect.bottom - windowRect.top, // 윈도우 세로 방향 해상도
			NULL, NULL, hInstance, pEngine);

		pEngine->Initialize();

		ShowWindow(m_hwnd, nShowCmd);

		// 메시지 루프
		MSG msg = {};
		while (msg.message != WM_QUIT)
		{
			// Process any messages in the queue.
			if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else
			{
				pEngine->UpdateGUI();

				pEngine->Update();
				pEngine->Render();
			}
		}

		return static_cast<int>(msg.wParam);
	}

	// 윈도우 프로시저
	LRESULT CALLBACK WinApp::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		EngineBase* pEngine = reinterpret_cast<EngineBase*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

		switch (uMsg) {
		case WM_CREATE:
		{
			// Save the DXSample* passed in to CreateWindow.
			LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
			SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
		}
		return 0;
		case WM_PAINT: // 화면 그릴 때
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hwnd, &ps);
			FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
			EndPaint(hwnd, &ps);
		}
		case WM_KEYDOWN: // 키가 눌렸을 때
			if (wParam == VK_ESCAPE) // ESC 키 확인
			{
				PostQuitMessage(0); // 메시지 큐에 종료 메시지 추가
			}
			break;
		case WM_SIZE: {
			break;
		}
		case WM_DESTROY: // 윈도우가 닫힐 때
			PostQuitMessage(0);
			return 0;

		return 0;
		}

		if (ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam))
			return true;

		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}


	std::wstring WinApp::GetLatestWinPixGpuCapturerPath()
	{
		LPWSTR programFilesPath = nullptr;
		SHGetKnownFolderPath(FOLDERID_ProgramFiles, KF_FLAG_DEFAULT, NULL, &programFilesPath);

		std::wstring pixSearchPath = programFilesPath + std::wstring(L"\\Microsoft PIX\\*");

		WIN32_FIND_DATA findData;
		bool foundPixInstallation = false;
		wchar_t newestVersionFound[MAX_PATH];

		HANDLE hFind = FindFirstFile(pixSearchPath.c_str(), &findData);
		if (hFind != INVALID_HANDLE_VALUE)
		{
			do
			{
				if (((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY) &&
					(findData.cFileName[0] != '.'))
				{
					if (!foundPixInstallation || wcscmp(newestVersionFound, findData.cFileName) <= 0)
					{
						foundPixInstallation = true;
						StringCchCopy(newestVersionFound, _countof(newestVersionFound), findData.cFileName);
					}
				}
			} while (FindNextFile(hFind, &findData) != 0);
		}

		FindClose(hFind);

		if (!foundPixInstallation)
		{
			// TODO: Error, no PIX installation found
		}

		wchar_t output[MAX_PATH];
		StringCchCopy(output, pixSearchPath.length(), pixSearchPath.data());
		StringCchCat(output, MAX_PATH, &newestVersionFound[0]);
		StringCchCat(output, MAX_PATH, L"\\WinPixGpuCapturer.dll");

		return &output[0];
	}
}