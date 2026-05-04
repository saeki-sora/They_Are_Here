#include "pch.h"
#include "EnemyState_Chase.h"
#include "EnemyState_Lost.h" 
#include "Enemy.h"
#include"EffectManager.h"
#include "FoundEffect.h"

using namespace DirectX::SimpleMath;

void EnemyChaseState::Enter(Enemy* enemy)
{
    //std::cout << "【追跡状態】開始\n";

    enemy->SetCurrentMaxSpeed(enemy->GetChaseSpeed()); // 追跡速度に設定

	// 経路更新タイマーをランダムに初期化して、複数の敵が同時に経路計算しないようにする
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(0.0f, 0.2f);

    m_PathUpdateTimer = -dist(gen);
    m_TimePlayerUnseen = 0.0f;

    // 初回の経路計算：プレイヤーの現在位置へ
    enemy->ComputePathTo(enemy->GetLastPlayerPos());

	EffectManager::GetInstance().StartEffect<FoundEffect>();// 発見エフェクト開始
}


void EnemyChaseState::Update(Enemy* enemy, float deltaTime)
{
    // タイマーを更新
    m_PathUpdateTimer += deltaTime;

    // プレイヤーが見えているかチェック
    bool canSeePlayer = enemy->CanSeePlayer();

    if (canSeePlayer)
    {
        // プレイヤーを視認中
        m_TimePlayerUnseen = 0.0f;

        // 一定間隔で経路を更新
        if (m_PathUpdateTimer >= m_PathUpdateInterval)
        {
            // プレイヤーの現在位置へ経路を再計算
            Vector3 playerPos = enemy->GetLastPlayerPos();

			if (enemy->ComputePathTo(playerPos))// 成功したらタイマーリセット
            {
                m_PathUpdateTimer = 0.0f;
            }
        }
    }
    else
    {
        // プレイヤーを見失っている：見失いタイマーを加算
        m_TimePlayerUnseen += deltaTime;
        //std::cout << "【追跡】プレイヤー視界外: " << m_TimePlayerUnseen << "秒経過\n";

		// 見失い時間が一定を超えたら見失い状態へ遷移
        if (m_TimePlayerUnseen >= enemy->GetLostStateDuration() || enemy->IsAtDestination())
        {
            enemy->ChangeState(std::make_unique<EnemyLostState>());
            return;
        }

        // 見失い中も経路更新は継続（最後に見た位置への追跡を継続）
        if (m_PathUpdateTimer >= m_PathUpdateInterval)
        {
            //std::cout << "【追跡】見失い中も経路更新：最後の位置へ\n";

            Vector3 lastPos = enemy->GetLastPlayerPos();
            if (enemy->ComputePathTo(lastPos))
            {
                // std::cout << "【追跡】見失い中経路更新成功\n";
                m_PathUpdateTimer = 0.0f;
            }
        }
    }

    // 旋回目標の設定：移動方向を向く
    Vector3 velocity = enemy->GetVelocity();
    if (velocity.LengthSquared() > 1e-6f)
    {
        float moveYaw = std::atan2(velocity.x, velocity.z);
        enemy->SetTargetYaw(moveYaw);
    }
}


void EnemyChaseState::Exit(Enemy* enemy)
{
    std::cout << "【追跡状態】終了\n";
}