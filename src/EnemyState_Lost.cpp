#include "pch.h"
#include "EnemyState_Lost.h"
#include "EnemyState_Search.h"
#include "EnemyState_Chase.h"
#include "Enemy.h"
#include "MakeMap.h"
#include "Pathfinder.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace DirectX::SimpleMath;

void EnemyLostState::Enter(Enemy* enemy)
{
    enemy->ClearPath();//現在のパスをクリア
    enemy->SetCurrentMaxSpeed(enemy->GetSearchSpeed());//探索速度に設定
    m_LostTimer = 0.0f;
    m_LookAroundTimer = 0.0f;
    m_LostStateDuration = enemy->GetLostStateDuration();//見失い状態の継続時間を取得
    m_OriginalYaw = enemy->GetRotation().y;
}

void EnemyLostState::Update(Enemy* enemy, float deltaTime)
{
    //プレイヤーを見つけたら追跡状態に遷移
    if (enemy->CanSeePlayer())
    {
        enemy->ChangeState(std::make_unique<EnemyChaseState>());
        return;
    }

    m_LostTimer += deltaTime;
    m_LookAroundTimer += deltaTime;

    const float lookSweepAngle = DirectX::XMConvertToRadians(55.0f); // 左右に何度振るか (中心から75度)
    const float lookSweepSpeed = 0.8f; // どのくらいの速さで振るか (1秒間に約1往復)

    //-1.0から1.0の往復
    float sweep = std::sin(m_LookAroundTimer * lookSweepSpeed * (float)M_PI);

    float newYaw = m_OriginalYaw + (sweep * lookSweepAngle);// 敵の新しい向きを計算

    Vector3 currentRot = enemy->GetRotation();
    currentRot.y = newYaw;
    enemy->SetRotation(currentRot);

    //見失い状態の継続時間を超えたら探索状態に戻る
    if (m_LostTimer > m_LostStateDuration)
    {
        // 完全に諦めて、通常の探索に戻る
        enemy->ChangeState(std::make_unique<EnemySearchState>());
        return;
    }

    ////目的地に到達したら、最後にプレイヤーを見た場所の周囲をランダムに歩き回る
    //if (enemy->IsAtDestination())
    //{
    //    

    //    //見失い時間が経過したら探索状態に戻る
    //    if (m_SearchTimeRemaining < 0.0f)
    //    {
    //        enemy->ChangeState(std::make_unique<EnemySearchState>());
    //        return;
    //    }
    //    else
    //    {
    //        MakeMap* map = enemy->GetMap();
    //        if (map)
    //        {
    //            //最後にプレイヤーを見た場所の周囲の歩行可能なセルを収集
    //            GridCoord centre = Pathfinder::WorldToGrid(map, enemy->GetLastPlayerPos());
    //            std::vector<GridCoord> local;

    //            // 半径2セルの範囲を探索
    //            for (int dx = -2; dx <= 2; dx++)
    //            {
    //                for (int dy = -2; dy <= 2; dy++)
    //                {
    //                    GridCoord c = { centre.x + dx, centre.y + dy };
    //                    if (map->IsWalkable(c.x, c.y))
    //                    {
    //                        local.push_back(c);
    //                    }
    //                }
    //            }
    //            if (!local.empty())
    //            {
    //                static std::random_device rd;
    //                static std::mt19937 rng(rd());
    //                std::uniform_int_distribution<size_t> dist(0, local.size() - 1);
    //                GridCoord targetCell = local[dist(rng)];
    //                Vector3 worldTarget = Pathfinder::GridToWorld(map, targetCell);
    //                enemy->ComputePathTo(worldTarget);
    //            }
    //        }
    //    }
    //}
}

void EnemyLostState::Exit(Enemy* enemy)
{
    // 元の向きに戻す
    Vector3 currentRot = enemy->GetRotation();
    currentRot.y = m_OriginalYaw;
    enemy->SetRotation(currentRot);
}