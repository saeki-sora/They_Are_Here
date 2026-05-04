#include "pch.h"
#include "EnemyState_StalkerChase.h"
#include "Enemy.h"
#include "Player.h"
#include "SceneManager.h"
#include "StalkerEnemy.h"
#include "EnemyState_Search.h"
#include "EffectManager.h"
#include "FoundEffect.h"

void EnemyState_StalkerChase::Enter(Enemy* enemy)
{
    // 追跡速度に設宁E
    enemy->SetIsChasing(true);
    enemy->SetCurrentMaxSpeed(enemy->GetChaseSpeed());
    m_PathUpdateTimer = 0.0f;
    m_TimeSinceLostSight = 0.0f;

	EffectManager::GetInstance().StartEffect<FoundEffect>();// 発見エフェクト開始    
}

void EnemyState_StalkerChase::Update(Enemy* enemy, float deltaTime)
{
    auto* stalker = static_cast<StalkerEnemy*>(enemy);

    m_PathUpdateTimer += deltaTime;

    auto playerWeak = SceneManager::GetInstance().FindObject<Player>();
    auto player = playerWeak.lock();

    if (!player || player->IsInvisible())
    {
        return; // プレイヤーがいなぁE��合�E征E��E
    }

    Vector3 playerPos = player->GetPosition();

    // -----------------------------------------------------------
    // 諦める判定ロジチE��
    // -----------------------------------------------------------

    // 位置はわかるが物琁E��に視線が通ってぁE��かをチェチE��
    bool hasLineOfSight = stalker->HasLineOfSight(stalker->GetPosition(), playerPos, 0.0f);

    if (hasLineOfSight)
    {
        m_TimeSinceLostSight = 0.0f;
    }
    else
    {
        // 壁�E裏などにぁE��姿が見えなぁE��ら、タイマ�Eを進める
        // 位置は透視してぁE��ので追ぁE��け続けるが、�E味は失われてぁE��
        m_TimeSinceLostSight += deltaTime;
    }

    // 一定時間姿を見てぁE��ければ諦める
    if (m_TimeSinceLostSight >= GIVE_UP_TIME)
    {
        // ロチE��オン解除
        stalker->ResetLockOn();

        // 探索モードに戻めE
        stalker->ChangeState(std::make_unique<EnemySearchState>());
        return; // 追跡終亁E
    }


    if (m_PathUpdateTimer >= enemy->GetChasePathUpdateInterval())
    {
        // 諦めてぁE��ぁE��は、正確な位置へパスを引く
        enemy->ComputePathTo(playerPos);
        m_PathUpdateTimer = 0.0f;
    }

    Vector3 velocity = enemy->GetVelocity();
    if (velocity.LengthSquared() > 0.001f)
    {
        float targetYaw = std::atan2(velocity.x, velocity.z);
        enemy->SetTargetYaw(targetYaw);
    }
}

void EnemyState_StalkerChase::Exit(Enemy* enemy)
{
    enemy->SetIsChasing(false);
}