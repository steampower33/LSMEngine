#include "WinApp.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

using namespace std;

HWND WinApp::m_hwnd = nullptr;

int WinApp::Run(EngineBase* pEngine, HINSTANCE hInstance, int nShowCmd)
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

	RECT windowRect = { 0, 0, static_cast<LONG>(pEngine->m_width), static_cast<LONG>(pEngine->m_height) };
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

			float deltaTime = ImGui::GetIO().DeltaTime;
			pEngine->Update(deltaTime);
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
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);
		FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
		EndPaint(hwnd, &ps);
		break;
	}
	case WM_LBUTTONDOWN:
	{
		if (!pEngine->m_leftButton)
			pEngine->m_dragStartFlag = true;
		pEngine->m_leftButton = true;
		break;
	}
	case WM_LBUTTONUP:
		pEngine->m_leftButton = false;
	case WM_MOUSEMOVE:
	{
		pEngine->m_isMouseMove = true;
		float mousePosX = static_cast<float>(GET_X_LPARAM(lParam));
		float mousePosY = static_cast<float>(GET_Y_LPARAM(lParam));
		pEngine->m_mouseDeltaX = mousePosX - pEngine->m_mousePosX;
		pEngine->m_mouseDeltaY = mousePosY - pEngine->m_mousePosY;
		pEngine->m_mousePosX = mousePosX;
		pEngine->m_mousePosY = mousePosY;
		pEngine->m_ndcX = (2.0f * static_cast<float>(mousePosX)) / static_cast<float>(pEngine->m_width) - 1.0f;
		pEngine->m_ndcY = 1.0f - (2.0f * static_cast<float>(mousePosY)) / static_cast<float>(pEngine->m_height);
		break;
	}
	case WM_KEYDOWN:
	{
		if (wParam == VK_ESCAPE)
		{
			PostQuitMessage(0);
		}
		if (wParam == 'F')
		{
			pEngine->m_camera->m_useFirstPersonView = !pEngine->m_camera->m_useFirstPersonView;
		}
		pEngine->m_camera->KeyDown(wParam);
		break;
	}
	case WM_KEYUP:
	{
		if (wParam == VK_ESCAPE)
		{
			PostQuitMessage(0);
		}
		pEngine->m_camera->KeyUp(wParam);
		break;
	}
	case WM_SIZE: {
		break;
	}
	case WM_DESTROY:
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
	wchar_t newestVersionFound[MAX_PATH] = L"";

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
		return L""; // 실패 시 빈 문자열 반환
	}

	wchar_t output[MAX_PATH];
	StringCchCopy(output, pixSearchPath.length(), pixSearchPath.data());
	StringCchCat(output, MAX_PATH, &newestVersionFound[0]);
	StringCchCat(output, MAX_PATH, L"\\WinPixGpuCapturer.dll");

	return std::wstring(output);
}