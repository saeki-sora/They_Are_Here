#pragma once
#include "Enemy.h"

class loversEnemy : public Enemy
{
public:

    loversEnemy(const DirectX::SimpleMath::Vector3& pos, const DirectX::SimpleMath::Vector3& size)
        : Enemy(pos, size) {
    }
    ~loversEnemy() override = default;

    void Init() override;
    void Update(float deltaTime) override;

    // パス計算
    bool ComputePathTo(const DirectX::SimpleMath::Vector3& target) override;

    // 相方を設定
    void SetPartner(std::shared_ptr<loversEnemy> partner) { m_Partner = partner; }

    // 相方から見つけた連絡を受ける関数
    void NotifyPlayerFound(const DirectX::SimpleMath::Vector3& playerPos);

    // 自分の現在のウェイポイントリストを相方に渡す用
    const std::deque<DirectX::SimpleMath::Vector3>& GetWaypoints() const { return m_Waypoints; }

    // 今、自分が裏取り役かどうか
    bool IsFlanking() const { return m_IsFlankingMode; }

    // 裏取りモードを強制的に設定する関数
    void SetFlankingMode(bool enable) { m_IsFlankingMode = enable; }

	// 共有されたターゲット位置を取得
    DirectX::SimpleMath::Vector3 GetSharedTargetPos() const { return m_SharedTargetPos; }

private:
    std::weak_ptr<loversEnemy> m_Partner; // 相方
    bool m_IsFlankingMode = false;          // trueなら裏取りルートを計算する
    DirectX::SimpleMath::Vector3 m_SharedTargetPos; // 共有されたターゲット位置
};