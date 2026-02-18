#include "pch.h"
#include "StalkerEnemy.h"
#include "EnemyState_StalkerChase.h" 
#include "Game.h"
#include "Player.h"

void StalkerEnemy::Init()
{
    //親の初期化をまず行う
    Enemy::Init();

    //ストーカー独自の設定で上書き
    m_ChaseSpeed = 20.0f;   // 遅い
    m_SearchSpeed = 20.0f;  // 探索も遅い
    m_CurrentMaxSpeed = m_SearchSpeed;

    // ライトを青色に設定
    m_SpotLightColor = DirectX::SimpleMath::Color(0.0f, 0.0f, 1.0f, 1.0f);

    m_IsLockedOn = false;
}

void StalkerEnemy::Update(float deltaTime)
{
    // まだロックオンしていない場合のみ、視界チェックを行う
    if (!m_IsLockedOn)
    {
        if (CanSeePlayer())
        {
            // 見つけた！ロックオン完了
            m_IsLockedOn = true;

			// 追跡状態に変更
            ChangeState(std::make_unique<EnemyState_StalkerChase>());
        }
    }

    // 親のUpdateを呼ぶ
    Enemy::Update(deltaTime);
}