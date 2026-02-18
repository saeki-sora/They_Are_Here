#pragma once
#include "ColliderObject.h"
#include "EnemyState.h"
#include "Pathfinder.h"
#include "SceneManager.h"
#include "SkinnedModel.h"

class Player;

namespace DirectX
{
    template<typename T> class PrimitiveBatch;
    struct VertexPositionColor;
}

class Enemy : public ColliderObject
{
public:

    Enemy(const DirectX::SimpleMath::Vector3& pos,
        const DirectX::SimpleMath::Vector3& size);
    ~Enemy() override;

    virtual void Init() override;
    virtual void Update(float deltaTime) override;
    void Draw() override;
    void Uninit() override;

    void SetMap(MakeMap* map) { m_Map = map; }

    void ChangeState(std::unique_ptr<EnemyState> newState);

    float GetCatchRange() const { return m_CatchRange; }//敵に捕まる範囲を取得

    //目的地までのパスを計算する
    virtual bool ComputePathTo(const DirectX::SimpleMath::Vector3& target);

    //パスに沿って移動する
    void FollowPath(float deltaTime);

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

    // パスをクリアしてその場で停止する
    void ClearPath()
    {
        m_Waypoints.clear();
        m_Velocity = DirectX::SimpleMath::Vector3::Zero;
    }


	void UpdatePlayerTracking(float deltaTime);//プレイヤーの位置履歴を更新する
	DirectX::SimpleMath::Vector3 PredictPlayerPosition() const;//プレイヤーの予測位置を計算する
	bool IsPlayerMovementStable() const;//プレイヤーの移動が安定しているかどうか
	float GetCurrentPathUpdateInterval() const;//現在のパス更新間隔を取得する
	bool ShouldUseDirectChase() const;//直接追跡モードを使用すべきかどうか
	void UpdateDirectChase();//直接追跡モードの更新処理

	static void ResetPathCalculationCount();// 今フレームのパス計算数をリセットする

    //プレイヤーが見えるかどうかを判定
    bool HasLineOfSight(const DirectX::SimpleMath::Vector3& a,
        const DirectX::SimpleMath::Vector3& b,
        float margin) const;


    //ゲッター
    float GetSearchSpeed() const { return m_SearchSpeed; }
    float GetChaseSpeed() const { return m_ChaseSpeed; }
    float GetChasePathUpdateInterval() const { return m_ChasePathUpdateInterval; }
    float GetLostStateDuration() const { return m_LostStateDuration; }
    DirectX::SimpleMath::Vector3 GetRotation() const { return m_Rotation; }
	bool GetUseDirectChase() const { return m_UseDirectChase; }//直接追跡モードの状態を取得
    DirectX::SimpleMath::Vector3 GetVelocity() const { return m_Velocity; }
    size_t GetWaypointCount() const { return m_Waypoints.size(); }

	//セッター
    void SetUseDirectChase(bool useDirectChase) { m_UseDirectChase = useDirectChase; }//直接追跡モードの状態を設定
    void SetRotation(const DirectX::SimpleMath::Vector3& rot) { m_Rotation = rot; }
    void SetCurrentMaxSpeed(float speed) { m_CurrentMaxSpeed = speed; }

private:

	SkinnedModel m_AnimatedModel;//アニメーションモデル

protected:

	void CheckForPlayerCollision();//プレイヤーとの衝突をチェックする

    // スムージングとウェイポイント化を行う共通関数
    void ProcessPath(const std::vector<DirectX::SimpleMath::Vector3>& worldPath);

    std::unique_ptr<EnemyState> m_State;
    std::deque<DirectX::SimpleMath::Vector3> m_Waypoints;//敵の移動ルート

    MakeMap* m_Map = nullptr;

	std::weak_ptr<Player> m_CachedPlayer;//プレイヤーオブジェクトへの弱参照

    std::unordered_set<int> m_Visited;//１度でも探索したターゲットを記録するためのセット

    GridCoord m_CurrentSearchTarget = { -1, -1 };//探索状態のとき、今向かってるターゲットの座標

    DirectX::SimpleMath::Vector3 m_LastPlayerPos = { 0.0f, 0.0f, 0.0f };//最後にプレイヤーがいた位置
    int m_SpotLightId = -1;

    float m_CurrentMaxSpeed=0.0f;//FollowPath()が現在使用する最高速度
    float m_DetectionRadius = 800.0f;//敵の視認半径
    float m_CatchRange = 30.0f;//敵に捕まる範囲
    float m_DetectionFOV = 90.0f;//敵の視野角
	float m_DetectionHeightLimit = 50.0f;//敵の視認高さ制限
    float m_FOVThreshold=0.0f;//敵の視野角のコサイン値がInitで計算され入る

    DirectX::SimpleMath::Vector3 m_Velocity{ 0,0,0 };// 旋回と平滑化のための運動パラメータ
    float m_SearchSpeed = 45.0f;    // 探索中の通常速度
    float m_ChaseSpeed = 70.0f;     // プレイヤー追跡時の速度（探索より速い）
    float m_ChasePathUpdateInterval = 0.25f; // 追跡中、何秒ごとに経路を再計算するか（しつこさ）
    float m_LostStateDuration = 5.0f;    // プレイヤーを見失った後、その場で何秒間捜索するか
    float m_MaxAccel = 60.0f;   //最高速に到達するまでの時間(大きいほど短くなる)
    float m_MaxAngVel = DirectX::XMConvertToRadians(320.0f); //旋回のキビキビ感（○○°/s）
    float m_LookAhead =4.0f;   // セグメント先読み距離
    float m_ArriveRadius = 20.0f;   // ウェイポイント終端の到達半径
    float m_WallMargin = 15.0f;  // 壁からの安全余白（半径）

    float m_PathCornerCutDistance = 15.0f; // コーナーをカットする距離(大きいほど手前から曲がる)

	static int m_PathRequestCount;// 現在のフレームでのパス要求数
    static int m_PathCalculationCount;      // 今フレームですでに計算した回数


    static const int MAX_CALCULATIONS_PER_FRAME = 1; // 1フレームに許可する最大回数

	float m_SpotLightRange = 1000.0f; // 敵のスポットライトの範囲
	float m_SpotLightAngleDeg = 60.0f; // 敵のスポットライトの角度（度）
	DirectX::SimpleMath::Color m_SpotLightColor = { 1.0f, 0.0f, 0.0f, 1.0f };// 敵のスポットライトの色


    // プレイヤー追跡用の履歴データ
    struct PlayerTrackingData
    {
        DirectX::SimpleMath::Vector3 position;
        DirectX::SimpleMath::Vector3 velocity;
        float timestamp = 0.0f;
    };


    struct RecentVisit
    {
        GridCoord coord;
        float timestamp;
    };

    std::deque<RecentVisit> m_RecentlyVisitedCells; // 最近訪問したセル
    static constexpr int MAX_RECENT_VISITS = 30; // 記憶する訪問履歴の数
    float m_GameTime = 0.0f; // ゲーム開始からの経過時間

    static constexpr int TRACKING_HISTORY_SIZE = 5;
    std::deque<PlayerTrackingData> m_PlayerHistory;//過去数フレームのプレイヤーの位置と速度

    // 予測航法用パラメータ
    float m_PredictionTime = 0.8f;              // プレイヤーの何秒先を予測するか
    float m_MinPredictionDistance = 30.0f;      // 予測が有効になる最小距離
    float m_MaxPredictionDistance = 500.0f;     // 予測が有効な最大距離

    // 適応的更新用パラメータ
    float m_PlayerDirectionChangeThreshold = 0.7f;  // 方向変化を検出する閾値（内積）
    float m_PlayerSpeedChangeThreshold = 15.0f;     // 速度変化を検出する閾値
    float m_StablePathUpdateInterval = 1.5f;        // プレイヤーが安定している時の更新間隔
    float m_UnstablePathUpdateInterval = 0.15f;     // プレイヤーが不安定な時の更新間隔

    // 内部状態
    bool m_IsPlayerMovementStable = true;
    float m_TimeSinceLastPathUpdate = 0.0f;

    bool m_UseDirectChase = false;  // 直接追跡モードの状態
    DirectX::SimpleMath::Vector3 m_DirectChaseTarget;  // 直接追跡時の目標位置

	// デバッグ用
    void DrawDebugWaypoints() const;// デバッグ用のウェイポイント描画関数
    void DrawDebugVision() const;// デバッグ用の視界描画関数

};
