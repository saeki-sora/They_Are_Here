#include "pch.h"
#include "Application.h"
#include "Renderer.h"
#include"Game.h"


//#include "imgui/imgui_impl_win32.h"
#define WIN32_LEAN_AND_MEAN

//extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
//#include "../imgui-master/imgui-master/backends/imgui_impl_win32.cpp"

const auto ClassName = TEXT("2024 framework ひな型");     //!< ウィンドウクラス名.
const auto WindowName = TEXT("2024 framework ひな型(フィールド描画)");    //!< ウィンドウ名.

HINSTANCE  Application::m_hInst;        // インスタンスハンドル
HWND       Application::m_hWnd;         // ウィンドウハンドル
uint32_t   Application::m_Width;        // ウィンドウの横幅
uint32_t   Application::m_Height;       // ウィンドウの縦幅

//-----------------------------------------------------------------------------
// コンストラクタ
//-----------------------------------------------------------------------------
Application::Application(uint32_t width, uint32_t height)
{
	m_Height = height;
	m_Width = width;

	timeBeginPeriod(1);
}

//-----------------------------------------------------------------------------
// デストラクタ
//-----------------------------------------------------------------------------
Application::~Application()
{
	timeEndPeriod(1);
}

//-----------------------------------------------------------------------------
// 実行
//-----------------------------------------------------------------------------
void Application::Run()
{
	if (InitApp())
	{
		MainLoop();
	}

	TermApp();
}

//-----------------------------------------------------------------------------
// 初期化処理
//-----------------------------------------------------------------------------
bool Application::InitApp()
{

#ifdef _DEBUG

	AllocConsole();

	FILE* fpOut;
	FILE* fpErr;

	freopen_s(&fpOut, "CONOUT$", "w", stdout);  // 標準出力をコンソールにバインド
	freopen_s(&fpErr, "CONOUT$", "w", stderr);  // エラー出力も

	std::cout.clear();
	std::cerr.clear();

	std::cout << "デバッグコンソール初期化完了" << std::endl;

#endif // DEBUG

	// ウィンドウの初期化.
	if (!InitWnd())
	{
		return false;
	}

	// 正常終了.
	return true;

}
//-----------------------------------------------------------------------------
// 終了処理
//-----------------------------------------------------------------------------
void Application::TermApp()
{
	// ウィンドウの終了処理.
	TermWnd();

#ifdef _DEBUG

	// コンソールを閉じる
	FreeConsole();

#endif // DEBUG

}

//-----------------------------------------------------------------------------
// ウィンドウの初期化処理
//-----------------------------------------------------------------------------
bool Application::InitWnd()
{
	// インスタンスハンドルを取得.
	auto hInst = GetModuleHandle(nullptr);
	if (hInst == nullptr)
	{
		return false;
	}

	// ウィンドウの設定.
	WNDCLASSEX wc = {};
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.hIcon = LoadIcon(hInst, IDI_APPLICATION);
	wc.hCursor = LoadCursor(hInst, IDC_ARROW);
	wc.hbrBackground = GetSysColorBrush(COLOR_BACKGROUND);
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = ClassName;
	wc.hIconSm = LoadIcon(hInst, IDI_APPLICATION);

	// ウィンドウの登録.
	if (!RegisterClassEx(&wc))
	{
		return false;
	}

	// インスタンスハンドル設定.
	m_hInst = hInst;

	// ウィンドウのサイズを設定.
	RECT rc = {};
	rc.right = static_cast<LONG>(m_Width);
	rc.bottom = static_cast<LONG>(m_Height);

	// ウィンドウサイズを調整.
	auto style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;
	AdjustWindowRect(&rc, style, FALSE);

	// ウィンドウを生成.
	m_hWnd = CreateWindowEx(
		0,
		//        WS_EX_TOPMOST,
		ClassName,
		WindowName,
		style,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		rc.right - rc.left,
		rc.bottom - rc.top,
		nullptr,
		nullptr,
		m_hInst,
		nullptr);

	if (m_hWnd == nullptr)
	{
		return false;
	}

	// ウィンドウを表示.
	ShowWindow(m_hWnd, SW_SHOWNORMAL);

	// ウィンドウを更新.
	UpdateWindow(m_hWnd);

	// ウィンドウにフォーカスを設定.
	SetFocus(m_hWnd);

	// 正常終了.
	return true;
}

//-----------------------------------------------------------------------------
// ウィンドウの終了処理
//-----------------------------------------------------------------------------
void Application::TermWnd()
{
	// ウィンドウの登録を解除.
	if (m_hInst != nullptr)
	{
		UnregisterClass(ClassName, m_hInst);
	}

	m_hInst = nullptr;
	m_hWnd = nullptr;
}

//-----------------------------------------------------------------------------
// メインループ
//-----------------------------------------------------------------------------
void Application::MainLoop()
{
	MSG msg = {};

	Game& game = Game::GetInstance();
	game.Init();


	// FPS計測用変数
	int fpsCounter = 0;
	auto lastFpsUpdateTime = std::chrono::high_resolution_clock::now();
	//long long oldTick = GetTickCount64(); // 前回計測時の時間
	//long long nowTick = oldTick; // 今回計測時の時間

	// FPS固定用変数
	LARGE_INTEGER liWork; // workがつく変数は作業用変数
	long long frequency;// どれくらい細かく時間をカウントできるか
	QueryPerformanceFrequency(&liWork);
	frequency = liWork.QuadPart;
	// 時間（単位：カウント）取得
	QueryPerformanceCounter(&liWork);
	long long oldCount = liWork.QuadPart;// 前回計測時の時間
	long long nowCount = oldCount;// 今回計測時の時間


	// ゲームループ
	while (1)
	{
		// 新たにメッセージがあれば
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			// ウィンドウプロシージャにメッセージを送る
			TranslateMessage(&msg);
			DispatchMessage(&msg);

			// 「WM_QUIT」メッセージを受け取ったらループを抜ける
			if (msg.message == WM_QUIT) {
				break;
			}
		}
		else
		{
			QueryPerformanceCounter(&liWork);// 現在時間を取得
			nowCount = liWork.QuadPart;
			// 1/60秒が経過したか？
			if (nowCount >= oldCount + frequency / 60) {

				// 経過秒数を計算
				double dt = static_cast<double>(nowCount - oldCount) / static_cast<double>(frequency);

				// 異常なスパイクを抑える
				if (dt > 0.1) dt = 0.1;//100ms上限

				// ゲーム処理実行
				game.Update(static_cast<float>(dt));

				game.Draw();


				fpsCounter++; // ゲーム処理を実行したら＋１する
				oldCount = nowCount;
			}
		}

		// 現在時刻を取得
		auto currentTime = std::chrono::high_resolution_clock::now();
		// 前回FPSを更新してからの経過時間（秒）
		std::chrono::duration<double> elapsed = currentTime - lastFpsUpdateTime;

		// 1秒以上経過していたらFPSを計算して表示
		if (elapsed.count() >= 1.0)
		{
			// FPSを計算
			double currentFps = static_cast<double>(fpsCounter) / elapsed.count();

			// タイトルバーに表示する文字列を作成
			TCHAR title[256];
			_stprintf_s(title, _countof(title), TEXT("%s (FPS: %.2f)"), WindowName, currentFps);

			// ウィンドウのタイトルバーを設定
			SetWindowText(m_hWnd, title);

			// カウンターと最終更新時間をリセット
			fpsCounter = 0;
			lastFpsUpdateTime = currentTime;
		}
	}

	game.Uninit();
}


//if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam)) return true;

  //ImGuiのメッセージ処理
//IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
//if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
//    return true;

//-----------------------------------------------------------------------------
// ウィンドウプロシージャ
//-----------------------------------------------------------------------------
LRESULT CALLBACK Application::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	//if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
 //       return true;
	switch (uMsg)
	{
	case WM_DESTROY:// ウィンドウ破棄のメッセージ
	{
		PostQuitMessage(0);// 「WM_QUIT」メッセージを送る　→　アプリ終了
	}
	break;

	case WM_CLOSE:  // 「x」ボタンが押されたら
	{
		int res = MessageBoxA(NULL, "終了しますか？", "確認", MB_OKCANCEL);
		if (res == IDOK) {

			Game::GetInstance().Uninit();
			DestroyWindow(hWnd);  // 「WM_DESTROY」メッセージを送る
		}
	}
	break;

	case WM_KEYDOWN: //キー入力があったメッセージ
	{
		// ESCキーの処理はCamera側で行うため、ここでは処理しない
		// 他のキーの処理が必要な場合はここに追加

	}
	break;

	default:
	{   // 受け取ったメッセージに対してデフォルトの処理を実行
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
		break;
	}
	}

	return 0;
}