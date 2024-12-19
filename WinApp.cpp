#include "WinApp.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace WindowApplication
{
	using namespace std;

	HWND WinApp::m_hwnd = nullptr;

	int WinApp::Run(EngineCore::EngineBase* pEngineBase, HINSTANCE hInstance, int nShowCmd)
	{
		// Check to see if a copy of WinPixGpuCapturer.dll has already been injected into the application.
		// This may happen if the application is launched through the PIX UI. 
		if (GetModuleHandle(L"WinPixGpuCapturer.dll") == 0)
		{
			LoadLibrary(GetLatestWinPixGpuCapturerPath().c_str());
		}

		WNDCLASSEX wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, L"LSMEngine", NULL };

		if (!RegisterClassEx(&wc)) return false;

		RECT wr = { 0, 0, 1280, 800 };
		AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, false);
		m_hwnd = CreateWindow(wc.lpszClassName, L"LSMEngineWindow",
			WS_OVERLAPPEDWINDOW,
			100, // 윈도우 좌측 상단의 x 좌표
			100, // 윈도우 좌측 상단의 y 좌표
			wr.right - wr.left, // 윈도우 가로 방향 해상도
			wr.bottom - wr.top, // 윈도우 세로 방향 해상도
			NULL, NULL, wc.hInstance, NULL);

		if (!m_hwnd) return false;

		pEngineBase->Init();

		ShowWindow(m_hwnd, nShowCmd);

		bool show_demo_window = true;
		bool show_another_window = false;
		ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

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

				ImGui_ImplDX12_NewFrame();
				ImGui_ImplWin32_NewFrame();
				ImGui::NewFrame();

				ImGui::Begin("Scene Control");
				ImGui::Text("Average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		
				pEngineBase->UpdateGUI();
				ImGui::End();

				// Rendering
				ImGui::Render();

				pEngineBase->Update();
				pEngineBase->Render();
			}
		}
		pEngineBase->Destroy();

		return static_cast<int>(msg.wParam);
	}

	// 윈도우 프로시저
	LRESULT CALLBACK WinApp::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		if (ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam))
			return true;

		switch (uMsg) {

		case WM_DESTROY: // 윈도우가 닫힐 때
			PostQuitMessage(0);
			return 0;

		case WM_PAINT: // 화면 그릴 때
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hwnd, &ps);
			FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
			EndPaint(hwnd, &ps);
		}
		return 0;
		}

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