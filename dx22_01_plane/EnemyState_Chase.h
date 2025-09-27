#pragma once
#include "EnemyState.h"

class EnemyChaseState : public EnemyState
{
private:

    float m_PathUpdateTimer = 0.0f;// Œo˜HÄŒvZ—pƒ^ƒCƒ}[

    float m_PathUpdateInterval = 0.5f;// 1•b‚²‚Æ‚ÉŒo˜H‚ğÄŒvZ

public:

    void Enter(Enemy* enemy) override;
    void Update(Enemy* enemy, float deltaTime) override;
    void Exit(Enemy* enemy) override;
};