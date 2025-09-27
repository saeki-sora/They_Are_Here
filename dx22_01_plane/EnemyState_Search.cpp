#include "EnemyState_Search.h"
#include "EnemyState_Chase.h"
#include "Enemy.h"

void EnemySearchState::Enter(Enemy* enemy)
{
    enemy->ChooseNextSearchTarget();
}

void EnemySearchState::Update(Enemy* enemy, float deltaTime)
{
    //プレイヤーが見えるかどうかをチェック
    if (enemy->CanSeePlayer())
    {
        enemy->ChangeState(std::make_unique<EnemyChaseState>());
        return;
    }

    //目的地に到着したら次の探索ターゲットを選ぶ
    if (enemy->IsAtDestination())
    {
        enemy->MarkTargetVisited();
        enemy->ChooseNextSearchTarget();
    }
}

void EnemySearchState::Exit(Enemy* enemy)
{

}