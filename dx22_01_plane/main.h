#pragma once

#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <windows.h>
#include <assert.h>
#include <functional>
#include <locale.h>
#include <string>

#pragma warning(push)
#pragma warning(disable:4005)

#pragma warning(pop)

#pragma comment (lib,"winmm.lib")

constexpr uint32_t SCREEN_WIDTH = 1280;//uint32==符号なし32ビット整数型
constexpr uint32_t SCREEN_HEIGHT = 720;//正確な型幅が必要な時に使う。あと軽い