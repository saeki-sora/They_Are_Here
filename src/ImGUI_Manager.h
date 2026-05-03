#pragma once

#include <d3d11.h>
#include <Windows.h>

class ImGUI_Manager
{
public:

    static void Init(HWND hwnd, ID3D11Device* device, ID3D11DeviceContext* context);
	static void DrawPanels();// 全パネルをまとめて描画する
    static void Render();
    static void Uninit();

	static bool IsVisible() { return s_Visible; }// デバッグUIが表示されているかどうか

private:

	static bool s_Initialized;// ImGuiの初期化が完了しているかどうか
	static bool s_Visible;// デバッグUIが表示されているかどうか

	static int  s_SelectedEnemyUID;// 現在選択されている敵のUID

	static void DrawEnemyParamPanel();// 敵のパラメータを表示するパネル
	static void DrawFSMPanel();// 敵の状態遷移を可視化するためのUI

	// 敵の状態遷移履歴をタイムライン形式で描画する
    static void DrawFSMGraph(const std::string& currentState);
    static void DrawTransitionTimeline(int uid);
};
