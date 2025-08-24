#pragma once
#include "EnemyState.h"

class EnemyLostState : public EnemyState
{
private:
    float m_SearchTimeRemaining = 0.0f;
public:
    void Enter(Enemy* enemy) override;
    void Update(Enemy* enemy, float deltaTime) override;
    void Exit(Enemy* enemy) override;
};