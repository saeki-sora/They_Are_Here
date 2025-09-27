#pragma once
#include "ColliderObject.h"
#include <memory>
#include <vector>
#include <deque>
#include <unordered_set>
#include "EnemyState.h"
#include "Pathfinder.h"

class Enemy : public ColliderObject
{

public:
    Enemy(const DirectX::SimpleMath::Vector3& pos,
        const DirectX::SimpleMath::Vector3& size);
    ~Enemy() override;

    void Init() override;
    void Update() override;
    void Draw() override;
    void Uninit() override;

    void SetMap(MakeMap* map) { m_Map = map; }

    void ChangeState(std::unique_ptr<EnemyState> newState);

    //目的地までのパスを計算する
    bool ComputePathTo(const DirectX::SimpleMath::Vector3& target);

    //パスに沿って移動する
    void FollowPath();

    //プレイヤーが見えるかどうかを判定する
    bool CanSeePlayer();

    //探索状態のときに次の探索ターゲットを選ぶ
    bool ChooseNextSearchTarget();

    //探索済みのターゲットをマークする
    void MarkTargetVisited();

    const DirectX::SimpleMath::Vector3& GetLastPlayerPos() const { return m_LastPlayerPos; }//最後にプレイヤーがいた位置を取得
    void SetLastPlayerPos(const DirectX::SimpleMath::Vector3& pos) { m_LastPlayerPos = pos; }

    MakeMap* GetMap() const { return m_Map; }
    bool HasPath() const { return !m_Waypoints.empty(); }//移動ルートがあるかどうか

    //目的地に到達したかどうか
    bool IsAtDestination() const { return m_Waypoints.empty(); }

private:

    void AdvanceWaypoints();

    ////プレイヤーが見えるかどうかを判定
    //bool HasLineOfSight(const DirectX::SimpleMath::Vector3& a,
    //    const DirectX::SimpleMath::Vector3& b, float margin) const;

    //プレイヤーが見えるかどうかを判定
    bool HasLineOfSight(const DirectX::SimpleMath::Vector3& a,
        const DirectX::SimpleMath::Vector3& b) const;

    std::unique_ptr<EnemyState> m_State;
    std::deque<DirectX::SimpleMath::Vector3> m_Waypoints;//敵の移動ルート

    MakeMap* m_Map = nullptr;

    std::unordered_set<int> m_Visited;//１度でも探索したターゲットを記録するためのセット

    GridCoord m_CurrentSearchTarget = { -1, -1 };//探索状態のとき、今向かってるターゲットの座標

    DirectX::SimpleMath::Vector3 m_LastPlayerPos = { 0.0f, 0.0f, 0.0f };//最後にプレイヤーがいた位置

    float m_MoveSpeed = 0.4f;//敵の移動速度
    float m_DetectionRadius = 80.0f;//敵の視認半径
    float m_FOVThreshold;//敵の視野角

    float m_MaxSpeed = 1.2f;   // 最高速度
    float m_MaxAccel = 8.0f;   // 直進加速度
    float m_MaxAngVel = DirectX::XMConvertToRadians(360.0f); // 最大角速度[rad/s]
    float m_LookAhead = 1.5f;   // セグメント先読み距離（移動1~2m相当）
    float m_ArriveRadius = 0.6f;   // ウェイポイント終端の到達半径
    float m_WallMargin = 0.35f;  // 壁からの安全余白（半径）
};
