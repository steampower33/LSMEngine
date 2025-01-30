#include "EngineBase.h"
#include "MainEngine.h"
#include "WinApp.h"

// EXE ���� �� ���� ��θ� ������ ������Ʈ ��Ʈ�� ����
void SetExecutionPathToProjectRoot() {
    // ���� ���� ���� EXE�� ��ü ��� ��������
    wchar_t exePath[MAX_PATH];
    GetModuleFileName(NULL, exePath, MAX_PATH);

    // EXE�� �ִ� ���� �������� ������Ʈ ��Ʈ(`LSMEngine`) ã��
    std::filesystem::path exeDir = std::filesystem::path(exePath).parent_path();
    std::filesystem::path projectRoot = exeDir.parent_path().parent_path();  // `x64/Release`���� �� �ܰ� ���� �̵�

    // ���� ��� ����
    std::wcout << L"[DEBUG] Changing execution path to: " << projectRoot << std::endl;
    std::filesystem::current_path(projectRoot);
}

int wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nShowCmd
) {
    SetExecutionPathToProjectRoot();

    AllocConsole(); // ���ο� �ܼ� â ����
    FILE* pStreamOut = nullptr;
    freopen_s(&pStreamOut, "CONOUT$", "w", stdout);

    MainEngine engine;
    int result = WinApp::Run(&engine, hInstance, nShowCmd);

    fclose(pStreamOut);
    FreeConsole();

    return result;
}

