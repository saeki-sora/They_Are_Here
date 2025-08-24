#include "EnemyState_Chase.h"
#include "EnemyState_Lost.h" 
#include "Enemy.h"
#include "Player.h"

using namespace DirectX::SimpleMath;

void EnemyChaseState::Enter(Enemy* enemy)
{
    m_RepathTimer = 0.0f;
    enemy->ComputePathTo(enemy->GetLastPlayerPos());
}

void EnemyChaseState::Update(Enemy* enemy, float deltaTime)
{
    bool seesPlayer = enemy->CanSeePlayer();
    if (seesPlayer)
    {
        m_RepathTimer += deltaTime;
        if (m_RepathTimer > 0.5f) // マジックナンバーの改善案も適用すると尚良い
        {
            m_RepathTimer = 0.0f;
            enemy->ComputePathTo(enemy->GetLastPlayerPos());
        }
    }
    else
    {
        enemy->ChangeState(std::make_unique<EnemyLostState>());
        return;
    }
}

void EnemyChaseState::Exit(Enemy* enemy)
{
    // 何もなし
}