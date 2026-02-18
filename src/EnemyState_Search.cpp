#include "pch.h"
#include "EnemyState_Search.h"
#include "EnemyState_Chase.h"
#include "Enemy.h"

void EnemySearchState::Enter(Enemy* enemy)
{
    // 状態が切り替わったら、必ず探索速度に戻す
    enemy->SetCurrentMaxSpeed(enemy->GetSearchSpeed());
    m_RetryTimer = 0.0f;
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

    if (enemy->IsAtDestination())
    {
        // リトライタイマーを進める
        m_RetryTimer += deltaTime;

        // 0.1秒ごとに再挑戦
        if (m_RetryTimer > 0.1f)
        {
            m_RetryTimer = 0.0f;

            if (enemy->ChooseNextSearchTarget())
            {
                enemy->MarkTargetVisited();
            }
        }
    }
    else
    {
		m_RetryTimer = 0.0f;// 目的地に到達していない場合はリトライタイマーをリセット
    }

    //旋回目標の設定
    Vector3 velocity = enemy->GetVelocity(); // 現在の移動速度を取得
    if (velocity.LengthSquared() > 1e-6f)
    {
        // 速度の方向をセット
        float moveYaw = std::atan2(velocity.x, velocity.z);
        enemy->SetTargetYaw(moveYaw);
    }
}

void EnemySearchState::Exit(Enemy* enemy)
{

}