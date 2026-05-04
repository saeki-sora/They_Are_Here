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

// ============================================================
// Enemy クラス
// ============================================================
class Enemy : public ColliderObject
{
public:

	Enemy(const DirectX::SimpleMath::Vector3& pos,
		const DirectX::SimpleMath::Vector3& size);
	~Enemy() override;

	virtual void Init() override;
	virtual void Update(float deltaTime) override;
	void Draw() override;
	void DrawShadow() override;
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

	bool IsChasing() const { return m_IsChasing; }
	void SetIsChasing(bool v) { m_IsChasing = v; }

	// パスをクリアしてその場で停止する
	void ClearPath()
	{
		m_Waypoints.clear();
		m_Velocity = DirectX::SimpleMath::Vector3::Zero;
	}


	static void ResetPathCalculationCount();// 今フレームのパス計算数をリセットする

	// JSONからパラメータを再読み込みして即座に反映する（ImGuiのApplyボタンから呼ばれる）
	void ReloadParams();

	// 壁を考慮した視線判定（marginは壁からの余裕距離）
	bool HasLineOfSight(const DirectX::SimpleMath::Vector3& a,
		const DirectX::SimpleMath::Vector3& b,
		float margin) const;

	// ウィスカー判定
	bool CheckWideLineOfSight(const DirectX::SimpleMath::Vector3& start, const DirectX::SimpleMath::Vector3& end, float radius) const;

	//ゲッター
	float GetSearchSpeed() const { return m_SearchSpeed; }
	float GetChaseSpeed() const { return m_ChaseSpeed; }
	float GetChasePathUpdateInterval() const { return m_ChasePathUpdateInterval; }
	float GetLostStateDuration() const { return m_LostStateDuration; }
	DirectX::SimpleMath::Vector3 GetRotation() const { return m_Rotation; }
	DirectX::SimpleMath::Vector3 GetVelocity() const { return m_Velocity; }

	//セッター
	void SetRotation(const DirectX::SimpleMath::Vector3& rot) { m_Rotation = rot; }
	void SetCurrentMaxSpeed(float speed) { m_CurrentMaxSpeed = speed; }

private:

	SkinnedModel m_AnimatedModel; // アニメーション付きモデル

	void UpdateColliderRotation();          // コライダーの回転を自身の向きに同期
	void UpdatePlayerCache();               // プレイヤーの弱参照キャッシュを再取得
	void UpdateVisitedCells(float deltaTime);// 現在のグリッドセルを訪問履歴に記録
	void UpdateRotation(float deltaTime);   // 旋回処理
	void UpdateSpotLight();                 // スポットライトの位置・向きを更新

protected:

	void CheckForPlayerCollision(); // プレイヤーとの衝突をチェックし、接触でゲームオーバー

	// パスを受け取り、スムージング・始点スキップ・コーナーカットを行う
	void ProcessPath(const std::vector<DirectX::SimpleMath::Vector3>& worldPath);

	std::unique_ptr<EnemyState> m_State;
	std::deque<DirectX::SimpleMath::Vector3> m_Waypoints;//敵の移動ルート

	MakeMap* m_Map = nullptr;

	std::weak_ptr<Player> m_CachedPlayer;//プレイヤーオブジェクトへの弱参照

	std::unordered_set<int> m_Visited;//１度でも探索したターゲットを記録するためのセット

	GridCoord m_CurrentSearchTarget = { -1, -1 };//探索状態のとき、今向かってるターゲットの座標

	DirectX::SimpleMath::Vector3 m_LastPlayerPos = { 0.0f, 0.0f, 0.0f };//最後にプレイヤーがいた位置
	int m_SpotLightId = -1;

	float m_CurrentMaxSpeed = 0.0f;//FollowPath()が現在使用する最高速度
	float m_DetectionRadius = 800.0f;//敵の視認半径
	float m_CatchRange = 30.0f;//敵に捕まる範囲
	float m_DetectionFOV = 90.0f;//敵の視野角
	float m_DetectionHeightLimit = 50.0f;//敵の視認高さ制限
	float m_FOVThreshold = 0.0f;//敵の視野角のコサイン値

	DirectX::SimpleMath::Vector3 m_Velocity{ 0,0,0 };// 旋回と平滑化のための運動パラメータ
	float m_SearchSpeed = 45.0f;    // 探索中の通常速度
	float m_ChaseSpeed = 50.0f;     // プレイヤー追跡時の速度（探索より速い）
	float m_ChasePathUpdateInterval = 0.25f; // 追跡中、何秒ごとに経路を再計算するか（しつこさ）
	float m_LostStateDuration = 5.0f;    // プレイヤーを見失った後、その場で何秒間捜索するか


	// ------ 移動・旋回パラメータ ------
	float m_MaxAccel = 60.0f;              // 加速度（大きいほど最高速に早く達する）
	float m_MaxAngVel = DirectX::XMConvertToRadians(320.0f); // 最大旋回速度（ラジアン/秒）
	float m_LookAhead = 4.0f;             // パス先読み距離（追跡時の滑らかさに影響）
	float m_ArriveRadius = 20.0f;         // ウェイポイント到達とみなす距離
	bool m_IsChasing = false;             // 追跡状態かどうか（最終ウェイポイントの到達半径を縮小するために使用）
	float m_WallMargin = 3.0f;            // 壁からの安全余白（敵の体のサイズから算出）

	float m_PathCornerCutDistance = 10.0f; // コーナーカット距離（大きいほど手前から曲がる）

	float m_BobPhase  = 0.0f;
	float m_BobOffset = 0.0f;

	// ------ パス計算フレームリミット ------
	static int m_PathCalculationCount;     // 現フレームのパス計算済み回数
	static const int MAX_CALCULATIONS_PER_FRAME = 1; // 1フレームに許可するパス計算の上限

	// ------ スポットライトパラメータ ------
	float m_SpotLightRange = 1000.0f;     // スポットライトの照射距離
	float m_SpotLightAngleDeg = 60.0f;    // スポットライトの照射角度（度）
	DirectX::SimpleMath::Color m_SpotLightColor = { 1.0f, 0.0f, 0.0f, 1.0f }; // スポットライトの色（赤）


	// ------ 訪問セル履歴構造体 ------
	// 敵が最近通ったグリッドセルと通過時刻を記録し、探索済みエリアを避ける
	struct RecentVisit
	{
		GridCoord coord;   // グリッド座標
		float timestamp;   // 訪問時のゲーム経過時間
	};

	std::deque<RecentVisit> m_RecentlyVisitedCells; // 最近訪問したセルの履歴
	static constexpr int MAX_RECENT_VISITS = 30;    // 保持する訪問履歴の最大数
	float m_GameTime = 0.0f;                        // ゲーム開始からの経過時間（秒）

	// ------ デバッグ識別子 ------
	// EnemyDebugData::Registry のキーとして使う。UID は生成順に振られる整数。
	int m_UID = -1;
	static int s_NextUID;


	// ------ デバッグ描画 ------
	void DrawDebugWaypoints() const; // 移動ルート（ウェイポイント）を描画
	void DrawDebugVision() const;    // 視界扇形を描画
	void DrawDebugWhiskerLines(); // ウィスカーをリアルタイム描画

};
