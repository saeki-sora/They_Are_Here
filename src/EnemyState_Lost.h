#pragma once
#include "EnemyState.h"

class EnemyLostState : public EnemyState
{
private:

    float m_LostTimer = 0.0f;//見失い状態のタイマー
    float m_LostStateDuration = 3.0f;//何秒間見失い状態を続けるか

    float m_OriginalYaw = 0.0f;     // この状態に入った時の敵の向き（Y軸回転）
    float m_LookAroundTimer = 0.0f; // キョロキョロ動作の内部タイマー

public:

    void Enter(Enemy* enemy) override;
    void Update(Enemy* enemy, float deltaTime) override;
    void Exit(Enemy* enemy) override;
};