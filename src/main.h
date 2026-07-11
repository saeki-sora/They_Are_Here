#pragma once

#define _CRT_SECURE_NO_WARNINGS


#pragma warning(push)
#pragma warning(disable:4005)

#pragma warning(pop)

#pragma comment (lib,"winmm.lib")

// デスクトップ解像度が取得できなかった場合のフォールバックウィンドウサイズ
// （通常は起動時にデスクトップ解像度のボーダーレスフルスクリーンになる）
constexpr uint32_t SCREEN_WIDTH = 1280;
constexpr uint32_t SCREEN_HEIGHT = 720;