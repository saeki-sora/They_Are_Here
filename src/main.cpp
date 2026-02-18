#include "pch.h"
#include "main.h"
#include "Application.h"

//=======================================
//エントリーポイント
//=======================================
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ wchar_t* lpCmdLine, _In_ int nShowCmd)
{
    std::stringstream ErrorLog{};

    // アプリケーション実行
    try
    {
        Application app(SCREEN_WIDTH, SCREEN_HEIGHT);
        app.Run();
    }
    catch (const std::exception& e)
    {
        // 標準的なエラーキャッチ
        ErrorLog << "Error Occurred: " << e.what() << std::endl;
        OutputDebugStringA(ErrorLog.str().c_str());
    }
    catch (...)
    {
        // その他全てのエラーキャッチ
        OutputDebugStringA("Unknown Error.");
    }

    return 0;
}