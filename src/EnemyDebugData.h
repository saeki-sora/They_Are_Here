#pragma once

#include <string>
#include <deque>
#include <unordered_map>

// 敵の状態遷移イベントを表す構造体
struct StateTransitionEvent
{
	std::string stateName; // 遷移先の状態名
	float       gameTime;  // 遷移が発生したゲーム内時間（秒）
};

// デバッグ用の敵状態履歴データ構造体
struct EnemyDebugData
{
    static constexpr int HISTORY_SIZE = 20;

    std::deque<StateTransitionEvent> history;
    std::string currentStateName = "None";

	// 敵UIDをキーとするグローバルレジストリ
    static std::unordered_map<int, EnemyDebugData>& Registry()
    {
        static std::unordered_map<int, EnemyDebugData> reg;
        return reg;
    }
};
