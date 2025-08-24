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

    bool ComputePathTo(const DirectX::SimpleMath::Vector3& target);//目的地までのパスを計算する

    void FollowPath(float deltaTime);//パスに沿って移動する

    bool CanSeePlayer();//プレイヤーが見えるかどうかを判定する

    bool ChooseNextSearchTarget();//探索状態のときに次の探索ターゲットを選ぶ

    void MarkTargetVisited();//探索済みのターゲットをマークする

    const DirectX::SimpleMath::Vector3& GetLastPlayerPos() const { return m_LastPlayerPos; }
    void SetLastPlayerPos(const DirectX::SimpleMath::Vector3& pos) { m_LastPlayerPos = pos; }

    MakeMap* GetMap() const { return m_Map; }
    bool HasPath() const { return !m_Waypoints.empty(); }
    bool IsAtDestination() const { return m_Waypoints.empty(); }//目的地に到達したかどうか

private:

    void AdvanceWaypoints();

    //プレイヤーが見えるかどうかを判定するためのヘルパー関数
    bool HasLineOfSight(const DirectX::SimpleMath::Vector3& a,
        const DirectX::SimpleMath::Vector3& b) const;

    std::unique_ptr<EnemyState> m_State;
    std::deque<DirectX::SimpleMath::Vector3> m_Waypoints;//敵の移動ルート

    MakeMap* m_Map = nullptr;

    std::unordered_set<int> m_Visited;//１度でも探索したターゲットを記録するためのセット

    GridCoord m_CurrentSearchTarget = { -1, -1 };//探索状態のとき、今向かってるターゲットの座標

    DirectX::SimpleMath::Vector3 m_LastPlayerPos = { 0.0f, 0.0f, 0.0f };

    float m_MoveSpeed = 0.4f;//敵の移動速度
    float m_DetectionRadius = 80.0f;//敵の視認半径
    float m_FOVThreshold = 0.7071f;
};
