#pragma once
#include "EnemyState.h"

class EnemyState_loversChase : public EnemyState
{
public:
    void Enter(Enemy* enemy) override;
    void Update(Enemy* enemy, float deltaTime) override;
    void Exit(Enemy* enemy) override;
    bool IsChaseState() const override { return true; }
private:
    float m_PathUpdateTimer = 0.0f;
};