//#include "ImGUI_Manager.h"
//#include <stdio.h>
//
//void ImGUI_Manager::InitImGui(HWND hwnd,ID3D11Device* g_pd3dDevice, ID3D11DeviceContext* g_pd3dDeviceContext)
//{
//    // ImGuiコンテキストの作成
//    IMGUI_CHECKVERSION();
//    ImGui::CreateContext();
//
//    // スタイル設定（オプション）
//    ImGui::StyleColorsDark();
//
//    // プラットフォーム/レンダラーバックエンドの初期化
//    ImGui_ImplWin32_Init(hwnd);
//    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
//}
//
//void ImGUI_Manager::RenderImGui()
//{
//
//    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
//
//    // ImGuiフレームの開始
//    ImGui_ImplDX11_NewFrame();
//    ImGui_ImplWin32_NewFrame();
//    ImGui::NewFrame();
//
//    // GUIの構築
//    ImGui::Begin("Example Window");           // ウィンドウ開始
//    ImGui::Text("Hello, world!");             // テキスト表示
//    if (ImGui::Button("Click Me"))            // ボタン
//    {
//        // ボタンが押されたときの処理
//        printf("Button Clicked!\n");
//    }
//    ImGui::SliderFloat("Slider", &sliderValue, 0.0f, 1.0f); // スライダー
//    ImGui::End();                           // ウィンドウ終了
//
//    // ImGuiフレームの終了
//    ImGui::Render();
//    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
//}
//
//void ImGUI_Manager::UninitImGui()
//{
//	// プラットフォーム/レンダラーバックエンドの終了
//	ImGui_ImplDX11_Shutdown();
//	ImGui_ImplWin32_Shutdown();
//
//	// ImGuiコンテキストの破棄
//	ImGui::DestroyContext();
//}