#include "EngineBase.h"
#include "MainEngine.h"
#include "WinApp.h"

int wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nShowCmd
) {
    EngineCore::MainEngine engine;

    return WindowApplication::WinApp::Run(&engine, hInstance, nShowCmd);;
}

