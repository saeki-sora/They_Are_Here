#include "pch.h"
#include "EnemyState_StalkerChase.h"
#include "Enemy.h"
#include "Player.h"
#include "SceneManager.h"
#include "StalkerEnemy.h"
#include "EnemyState_Search.h"
#include "EffectManager.h"
#include "FoundEffect.h"
#include "SoundManager.h"

void EnemyState_StalkerChase::Enter(Enemy* enemy)
{
	// 追跡モードに入るときは、現在の位置からプレイヤーへのパスを引いておく
    enemy->SetCurrentMaxSpeed(enemy->GetChaseSpeed());
    m_PathUpdateTimer = 0.0f;
    m_TimeSinceLostSight = 0.0f;

	EffectManager::GetInstance().StartEffect<FoundEffect>();// 発見エフェクト開始
	SoundManager::GetInstance().PlaySE("SE_Found");
}

void EnemyState_StalkerChase::Update(Enemy* enemy, float deltaTime)
{
    auto* stalker = static_cast<StalkerEnemy*>(enemy);

    m_PathUpdateTimer += deltaTime;

    auto playerWeak = SceneManager::GetInstance().FindObject<Player>();
    auto player = playerWeak.lock();

    if (!player || player->IsInvisible())
    {
        return; // プレイヤーがいないか、不可視状態
    }

    Vector3 playerPos = player->GetPosition();

    // -----------------------------------------------------------
    // 諦める判定ロジック
    // -----------------------------------------------------------

    // 位置はわかるが物体に視線が通っているかをチェック
    bool hasLineOfSight = stalker->HasLineOfSight(stalker->GetPosition(), playerPos, 0.0f);

    if (hasLineOfSight)
    {
        m_TimeSinceLostSight = 0.0f;
    }
    else
    {
        // 壁の裏などに姿が見えなくなったら、タイマーを進める
        // 位置は透視しているので追い続けるが、視線は失われている
        m_TimeSinceLostSight += deltaTime;
    }

    // 一定時間姿を見ていなければ諦める
    if (m_TimeSinceLostSight >= GIVE_UP_TIME)
    {
        // ロックオン解除
        stalker->ResetLockOn();

        // 探索モードに戻す
        stalker->ChangeState(std::make_unique<EnemySearchState>());
        return; // 追跡終了
    }


    if (m_PathUpdateTimer >= enemy->GetChasePathUpdateInterval())
    {
		// 定期的にパスを再計算して、プレイヤーの動きに追従する
        enemy->ComputePathTo(playerPos);
        m_PathUpdateTimer = 0.0f;
    }

	// 目的地に向かって移動する
    Vector3 velocity = enemy->GetVelocity();
    if (velocity.LengthSquared() > 0.001f)
    {
        float targetYaw = std::atan2(velocity.x, velocity.z);
        enemy->SetTargetYaw(targetYaw);
    }
}

void EnemyState_StalkerChase::Exit(Enemy* enemy)
{
}