#include "EnemyState_Lost.h"
#include "EnemyState_Search.h"
#include "EnemyState_Chase.h"
#include "Enemy.h"
#include "MakeMap.h"
#include "Pathfinder.h"
#include <random>

using namespace DirectX::SimpleMath;

void EnemyLostState::Enter(Enemy* enemy)
{
    m_SearchTimeRemaining = 10.0f;
    enemy->ComputePathTo(enemy->GetLastPlayerPos());
}

void EnemyLostState::Update(Enemy* enemy, float deltaTime)
{
    //プレイヤーを見つけたら追跡状態に遷移
    if (enemy->CanSeePlayer())
    {
        enemy->ChangeState(std::make_unique<EnemyChaseState>());
        return;
    }

    //目的地に到達したら、最後にプレイヤーを見た場所の周囲をランダムに歩き回る
    if (enemy->IsAtDestination())
    {
        m_SearchTimeRemaining -= deltaTime;

        //見失い時間が経過したら探索状態に戻る
        if (m_SearchTimeRemaining < 0.0f)
        {
            enemy->ChangeState(std::make_unique<EnemySearchState>());
            return;
        }
        else
        {
            MakeMap* map = enemy->GetMap();
            if (map)
            {
                //最後にプレイヤーを見た場所の周囲の歩行可能なセルを収集
                GridCoord centre = Pathfinder::WorldToGrid(map, enemy->GetLastPlayerPos());
                std::vector<GridCoord> local;

                // 半径2セルの範囲を探索
                for (int dx = -2; dx <= 2; dx++)
                {
                    for (int dy = -2; dy <= 2; dy++)
                    {
                        GridCoord c = { centre.x + dx, centre.y + dy };
                        if (map->IsWalkable(c.x, c.y))
                        {
                            local.push_back(c);
                        }
                    }
                }
                if (!local.empty())
                {
                    static std::random_device rd;
                    static std::mt19937 rng(rd());
                    std::uniform_int_distribution<size_t> dist(0, local.size() - 1);
                    GridCoord targetCell = local[dist(rng)];
                    Vector3 worldTarget = Pathfinder::GridToWorld(map, targetCell);
                    enemy->ComputePathTo(worldTarget);
                }
            }
        }
    }
}

void EnemyLostState::Exit(Enemy* enemy)
{
   
}