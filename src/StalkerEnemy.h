#pragma once
#include "Enemy.h"

class StalkerEnemy : public Enemy
{
public:

    StalkerEnemy(const DirectX::SimpleMath::Vector3& pos, const DirectX::SimpleMath::Vector3& size)
        : Enemy(pos, size) {
    }

    ~StalkerEnemy() override = default;

    void Init() override;
    void Update(float deltaTime) override;

    // ロックオン状態かどうかを取得
    bool IsLockedOn() const { return m_IsLockedOn; }

    void ResetLockOn() { m_IsLockedOn = false; }//追跡を諦める

private:

    bool m_IsLockedOn = false; //ストーカーになる
};