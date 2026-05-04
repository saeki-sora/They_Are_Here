#pragma once
#include "EnemyState.h"

class EnemyChaseState : public EnemyState
{
private:
    float m_PathUpdateTimer = 0.0f;      // 経路再計算用タイマー
    float m_PathUpdateInterval = 0.5f;   // 0.5秒ごとに経路を再計算
    float m_TimePlayerUnseen = 0.0f;     // プレイヤーを見失ってからの経過時間

public:
    void Enter(Enemy* enemy) override;
    void Update(Enemy* enemy, float deltaTime) override;
    void Exit(Enemy* enemy) override;
    bool IsChaseState() const override { return true; }
};