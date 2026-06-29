#pragma once

#include <d3d11.h>
#include <Windows.h>

class ImGUI_Manager
{
public:

    static void Init(HWND hwnd, ID3D11Device* device, ID3D11DeviceContext* context);
    static void BeginFrame();// フレーム開始（NewFrame 呼び出し）
	static void DrawPanels();// 全パネルをまとめて描画する
    static void Render();
    static void Uninit();

	static bool IsVisible() { return s_Visible; }// フラグUIの表示状態を取得

private:

	static bool s_Initialized;// ImGuiの初期化状態
	static bool s_Visible;// フラグUIの表示状態

	static int  s_SelectedEnemyUID;// 選択中の敵のUID

	static void DrawEnemyParamPanel();// 敵のパラメータを表示するパネル
	static void DrawFSMPanel();// 敵のFSMを表示するパネル
	static void DrawGraphicsPanel();// グラフィック設定を表示するパネル

	// FSMグラフを描画
    static void DrawFSMGraph(const std::string& currentState);
    static void DrawTransitionTimeline(int uid);
};
