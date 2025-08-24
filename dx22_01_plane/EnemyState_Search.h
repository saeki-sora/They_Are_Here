#pragma once
#include "EnemyState.h"

class EnemySearchState : public EnemyState
{
public:
    void Enter(Enemy* enemy) override;
    void Update(Enemy* enemy, float deltaTime) override;
    void Exit(Enemy* enemy) override;
};
