#pragma once
#include "ColliderObject.h"
#include"MakeMap.h"

// 前方宣言
class Player;
class Enemy;

class GoalKey : public ColliderObject
{
public:
    GoalKey(const DirectX::SimpleMath::Vector3& pos, const DirectX::SimpleMath::Vector3& size);
    ~GoalKey() override;

    void Init() override;
    void Update(float deltaTime) override;
    void Draw() override;
    void Uninit() override;

    // 追従対象を設定する（敵またはプレイヤー）
    void SetTarget(std::weak_ptr<ColliderObject> target);

    // プレイヤーが取得済みかどうか
    bool IsObtained() const { return m_IsObtained; }

private:
    // --- パラメータ（調整可能） ---
    // ターゲットの背中からどれくらい離れるか
    static constexpr float FOLLOW_DISTANCE = MAP::Config::BLOCK_SIZE; // ブロックサイズの1個分後ろ
    
    static constexpr float HEIGHT_OFFSET = 20.0f;// 浮遊感
    
    static constexpr float LERP_FACTOR = 0.1f;// 追従の滑らかさ（小さいほど遅れてついてくる）

    // --- 内部変数 ---
    std::weak_ptr<ColliderObject> m_Target; // 追従対象
    bool m_IsObtained = false;              // プレイヤーが取得したかフラグ

    // プレイヤーへの弱参照（当たり判定用）
    std::weak_ptr<Player> m_CachedPlayer;

    // 追従処理
    void FollowTarget(float deltaTime);

    // プレイヤーとの接触判定
    void CheckCollisionWithPlayer();
};