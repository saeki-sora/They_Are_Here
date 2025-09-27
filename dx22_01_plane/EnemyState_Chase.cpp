#include "EnemyState_Chase.h"
#include "EnemyState_Lost.h" 
#include "Enemy.h"
#include "Player.h"

using namespace DirectX::SimpleMath;

void EnemyChaseState::Enter(Enemy* enemy)
{
    m_PathUpdateTimer = 0.0f;
    m_PathUpdateInterval= 0.5f; //0.5•b‚²‚Æ‚ÉƒpƒX‚ðÄŒvŽZ‚·‚é
    enemy->ComputePathTo(enemy->GetLastPlayerPos());
}

void EnemyChaseState::Update(Enemy* enemy, float deltaTime)
{
    bool seePlayer = enemy->CanSeePlayer();

    if (seePlayer)
    {
        m_PathUpdateTimer += deltaTime;

        //0.5•b‚²‚Æ‚ÉƒpƒX‚ðÄŒvŽZ‚·‚é
        if (m_PathUpdateTimer > m_PathUpdateInterval)
        {
            m_PathUpdateTimer = 0.0f;
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

}