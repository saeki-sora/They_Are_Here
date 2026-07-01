#pragma once

// ゲーム結果データを画面間で受け渡すシンプルなデータホルダー
struct ResultData
{
	static float s_ElapsedTime; // プレイ時間（秒）。Stage1Scene が毎フレーム更新する
};
