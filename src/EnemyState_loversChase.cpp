#include "pch.h"
#include "EnemyState_loversChase.h"
#include "loversEnemy.h"
#include "EnemyState_Search.h"
#include "Game.h"
#include "Player.h"
#include "EffectManager.h"
#include "FoundEffect.h"
#include "SoundManager.h"

void EnemyState_loversChase::Enter(Enemy* enemy)
{
    enemy->SetCurrentMaxSpeed(enemy->GetChaseSpeed());
    m_PathUpdateTimer = 0.0f;

	EffectManager::GetInstance().StartEffect<FoundEffect>();// 発見エフェクト開始
	SoundManager::GetInstance().PlaySE("SE_Found");
}


void EnemyState_loversChase::Update(Enemy* enemy, float deltaTime)
{
    auto* flanker = static_cast<loversEnemy*>(enemy);
    m_PathUpdateTimer += deltaTime;


    if (flanker->IsFlanking() && flanker->IsAtDestination())
    {
        // 裏取り完了。モードを解除する。
        flanker->SetFlankingMode(false);

        // 到着したのにプレイヤーが見えていないなら、探索状態へ移行
        if (!flanker->CanSeePlayer())
        {
            enemy->ChangeState(std::make_unique<EnemySearchState>());
            return;
        }
    }

    // ターゲット位置の決定
    DirectX::SimpleMath::Vector3 targetPos;
    bool hasTarget = false;

    // 自分で見えているなら、プレイヤーの現在位置
    if (flanker->CanSeePlayer())
    {
        targetPos = flanker->GetLastPlayerPos(); // CanSeePlayer内で更新される
        hasTarget = true;
        // ※loversEnemy::Updateで「見えたらm_IsFlankingMode = false」にしているので、
        // 次回のComputePathToは通常ルート（最短）になります。
    }
    // 見えていないが、裏取りモードなら共有位置
    else if (flanker->IsFlanking())
    {
        targetPos = flanker->GetSharedTargetPos();
        hasTarget = true;
    }

    if (!hasTarget)
    {
        // ターゲットが見えなくても、まだ移動中（目的地に着いていない）なら諦めない
        if (!enemy->IsAtDestination())
        {
            return; // ここで return することで、下のパス再計算を行わずに移動(FollowPath)だけ継続させる
        }

        // 目的地に到着済み、かつターゲットもいないなら、探索へ戻る
        enemy->ChangeState(std::make_unique<EnemySearchState>());
        return;
    }

    // パス更新
    if (m_PathUpdateTimer >= enemy->GetChasePathUpdateInterval())
    {
        // ComputePathTo がオーバーライドされているので、
        // IsFlanking() が true なら自動的に「相方を避けるルート」が計算される
        flanker->ComputePathTo(targetPos);
        m_PathUpdateTimer = 0.0f;
    }

    // 移動方向を向く
    auto vel = enemy->GetVelocity();
    if (vel.LengthSquared() > 0.001f)
    {
        float yaw = std::atan2(vel.x, vel.z);
        enemy->SetTargetYaw(yaw);
    }
}

void EnemyState_loversChase::Exit(Enemy* enemy)
{
}