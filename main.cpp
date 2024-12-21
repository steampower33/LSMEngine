#include "EngineBase.h"
#include "WinApp.h"

using namespace std;

FILE* CreateConsoleAndPrintDemoInformation()
{
    AllocConsole();
    FILE* pStreamOut = nullptr;
    freopen_s(&pStreamOut, "CONOUT$", "w", stdout);

    cout << "Using Console" << endl;

    return pStreamOut;
}

void DestroyConsole(FILE* pStream)
{
    fclose(pStream);
    FreeConsole();
}


int wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nShowCmd
) {
    EngineCore::EngineBase engine(1280, 800, L"LSMEngine");

    FILE* pStreamOut = CreateConsoleAndPrintDemoInformation();
    int result = WindowApplication::WinApp::Run(&engine, hInstance, nShowCmd);
    DestroyConsole(pStreamOut);
    return result;
}

