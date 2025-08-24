#pragma once
#include "EnemyState.h"

class EnemyChaseState : public EnemyState
{
private:
    float m_RepathTimer = 0.0f;
public:
    void Enter(Enemy* enemy) override;
    void Update(Enemy* enemy, float deltaTime) override;
    void Exit(Enemy* enemy) override;
};