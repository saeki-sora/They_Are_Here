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
    m_SearchTimeRemaining = 3.0f; // マジックナンバーの改善案も適用すると尚良い
    enemy->ComputePathTo(enemy->GetLastPlayerPos());
}

void EnemyLostState::Update(Enemy* enemy, float deltaTime)
{
    if (enemy->CanSeePlayer())
    {
        enemy->ChangeState(std::make_unique<EnemyChaseState>());
        return;
    }

    if (enemy->IsAtDestination())
    {
        m_SearchTimeRemaining -= deltaTime;
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
                GridCoord centre = Pathfinder::WorldToGrid(map, enemy->GetLastPlayerPos());
                std::vector<GridCoord> local;
                for (int dx = -2; dx <= 2; ++dx)
                {
                    for (int dy = -2; dy <= 2; ++dy)
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
    // 何もなし
}