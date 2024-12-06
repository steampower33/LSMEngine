#include "stdafx.h"
#include "RenderApp.h"

using namespace std;

int wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nShowCmd)
{
    EngineCore::RenderApp app;

    if (!app.Initialize())
    {
        cout << "Initialization failed." << endl;
        return -1;
    }

    return app.Run();
}