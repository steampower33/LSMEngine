#include "EngineBase.h"
#include "MainEngine.h"
#include "WinApp.h"

// EXE 실행 시 실행 경로를 강제로 프로젝트 루트로 변경
void SetExecutionPathToProjectRoot() {
    // 현재 실행 중인 EXE의 전체 경로 가져오기
    wchar_t exePath[MAX_PATH];
    GetModuleFileName(NULL, exePath, MAX_PATH);

    // EXE가 있는 폴더 기준으로 프로젝트 루트(`LSMEngine`) 찾기
    std::filesystem::path exeDir = std::filesystem::path(exePath).parent_path();
    std::filesystem::path projectRoot = exeDir.parent_path().parent_path();  // `x64/Release`에서 두 단계 위로 이동

    // 실행 경로 변경
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

    AllocConsole(); // 새로운 콘솔 창 생성
    FILE* pStreamOut = nullptr;
    freopen_s(&pStreamOut, "CONOUT$", "w", stdout);

    MainEngine engine;
    int result = WinApp::Run(&engine, hInstance, nShowCmd);

    fclose(pStreamOut);
    FreeConsole();

    return result;
}

