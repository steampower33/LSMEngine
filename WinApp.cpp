#include "WinApp.h"

namespace WindowApplication
{
    int WinApp::Run(HINSTANCE hInstance, int nShowCmd)
    {
        // 윈도우 클래스 정의
        const wchar_t CLASS_NAME[] = L"SampleWindowClass";

        WNDCLASS wc = {};
        wc.lpfnWndProc = WindowProc;          // 윈도우 프로시저 지정
        wc.hInstance = hInstance;            // 인스턴스 핸들
        wc.lpszClassName = CLASS_NAME;       // 클래스 이름

        RegisterClass(&wc);

        // 윈도우 생성
        HWND hwnd = CreateWindowEx(
            0,                              // 확장 스타일
            CLASS_NAME,                     // 클래스 이름
            L"LSMEngine",                   // 윈도우 제목
            WS_OVERLAPPEDWINDOW,            // 윈도우 스타일
            CW_USEDEFAULT, CW_USEDEFAULT,   // 초기 위치
            CW_USEDEFAULT, CW_USEDEFAULT,   // 초기 크기
            nullptr,                        // 부모 윈도우
            nullptr,                        // 메뉴
            hInstance,                      // 인스턴스 핸들
            nullptr                         // 추가 매개변수
        );

        if (!hwnd) return 0;

        ShowWindow(hwnd, nShowCmd);

        // 메시지 루프
        MSG msg = {};
        while (GetMessage(&msg, nullptr, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        return static_cast<int>(msg.wParam);
    }

    // 윈도우 프로시저
    LRESULT CALLBACK WinApp::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
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
}