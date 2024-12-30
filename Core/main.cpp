#include "EngineBase.h"
#include "MainEngine.h"
#include "WinApp.h"

int wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nShowCmd
) {

    AllocConsole(); // ���ο� �ܼ� â ����
    FILE* pStreamOut = nullptr;
    freopen_s(&pStreamOut, "CONOUT$", "w", stdout);

    MainEngine engine;
    int result = WinApp::Run(&engine, hInstance, nShowCmd);

    fclose(pStreamOut);
    FreeConsole();

    return result;
}

