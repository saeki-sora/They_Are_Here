#pragma once
#include "EnemyState.h"

class EnemyLostState : public EnemyState
{
private:

    float m_SearchTimeRemaining = 10.0f;//Œ©¸‚¢ó‘Ô‚Ìc‚èŠÔ

public:

    void Enter(Enemy* enemy) override;
    void Update(Enemy* enemy, float deltaTime) override;
    void Exit(Enemy* enemy) override;
};