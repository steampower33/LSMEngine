#include "stdafx.h"
#include "EngineCore.h"

int wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nShowCmd)
{
    EngineCore::AppBase app;

	return app.RunApplication(hInstance, nShowCmd);
}