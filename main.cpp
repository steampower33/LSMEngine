#include "WinApp.h"

using namespace WindowApplication;

int wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nShowCmd
) {
    return WinApp::Run(hInstance, nShowCmd);
}

