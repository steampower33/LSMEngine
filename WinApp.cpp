#include "WinApp.h"

namespace WindowApplication
{
    HWND WinApp::m_hwnd = nullptr;

    int WinApp::Run(EngineCore::EngineBase* pEngineBase, HINSTANCE hInstance, int nShowCmd)
    {
        // ������ Ŭ���� ����
        const wchar_t CLASS_NAME[] = L"SampleWindowClass";

        WNDCLASS wc = {};
        wc.lpfnWndProc = WindowProc;          // ������ ���ν��� ����
        wc.hInstance = hInstance;            // �ν��Ͻ� �ڵ�
        wc.lpszClassName = CLASS_NAME;       // Ŭ���� �̸�

        RegisterClass(&wc);

        // ������ ����
        m_hwnd = CreateWindowEx(
            0,                              // Ȯ�� ��Ÿ��
            CLASS_NAME,                     // Ŭ���� �̸�
            L"LSMEngine",                   // ������ ����
            WS_OVERLAPPEDWINDOW,            // ������ ��Ÿ��
            CW_USEDEFAULT, CW_USEDEFAULT,   // �ʱ� ��ġ
            CW_USEDEFAULT, CW_USEDEFAULT,   // �ʱ� ũ��
            nullptr,                        // �θ� ������
            nullptr,                        // �޴�
            hInstance,                      // �ν��Ͻ� �ڵ�
            nullptr                         // �߰� �Ű�����
        );

        if (!m_hwnd) return 0;

        pEngineBase->Init();

        ShowWindow(m_hwnd, nShowCmd);

        // �޽��� ����
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
                pEngineBase->Update();
                pEngineBase->Render();
            }
        }
        pEngineBase->Destroy();

        return static_cast<int>(msg.wParam);
    }

    // ������ ���ν���
    LRESULT CALLBACK WinApp::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        switch (uMsg) {
        case WM_DESTROY: // �����찡 ���� ��
            PostQuitMessage(0);
            return 0;

        case WM_PAINT: // ȭ�� �׸� ��
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