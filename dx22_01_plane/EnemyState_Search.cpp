#include "EnemyState_Search.h"
#include "EnemyState_Chase.h"
#include "Enemy.h"

// EnemySearchState‚ÌŽÀ‘•‚¾‚¯‚ð‚±‚±‚É’u‚­
void EnemySearchState::Enter(Enemy* enemy)
{
    enemy->ChooseNextSearchTarget();
}

void EnemySearchState::Update(Enemy* enemy, float deltaTime)
{
    if (enemy->CanSeePlayer())
    {
        enemy->ChangeState(std::make_unique<EnemyChaseState>());
        return;
    }
    if (enemy->IsAtDestination())
    {
        enemy->MarkTargetVisited();
        enemy->ChooseNextSearchTarget();
    }
}

void EnemySearchState::Exit(Enemy* enemy)
{

}