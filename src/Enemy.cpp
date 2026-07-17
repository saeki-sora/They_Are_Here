#include "pch.h"
#include "Enemy.h"
#include "dx11helper.h"
#include "Game.h"
#include "MakeMap.h"
#include "Player.h"
#include "DebugRenderer.h"
#include "Renderer.h"
#include "StaticMesh.h"
#include "utility.h"
#include "SimpleBoxCollider.h"
#include "EnemyState_Search.h"
#include "ConfigManager.h"
#include "GameOverScene.h"
#include "FadeTransition.h"
#include "DebugManager.h"
#include "EnemyDebugData.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;


//今フレームにすでにパスを計算した回数
int Enemy::m_PathCalculationCount = 0;

// 生成順に振るUID。スポーン順が分かるのでデバッグ時に判別しやすい
int Enemy::s_NextUID = 0;

// フレームごとにリセット
void Enemy::ResetPathCalculationCount() { m_PathCalculationCount = 0; }

//=======================================
// ゲッター・セッター
//=======================================
void Enemy::SetMap(MakeMap* map) { m_Map = map; }
float Enemy::GetCatchRange() const { return m_CatchRange; }
const DirectX::SimpleMath::Vector3& Enemy::GetLastPlayerPos() const { return m_LastPlayerPos; }
void Enemy::SetLastPlayerPos(const DirectX::SimpleMath::Vector3& pos) { m_LastPlayerPos = pos; }
MakeMap* Enemy::GetMap() const { return m_Map; }
bool Enemy::HasPath() const { return !m_Waypoints.empty(); }
bool Enemy::IsAtDestination() const { return m_Waypoints.empty(); }

void Enemy::ClearPath()
{
	m_Waypoints.clear();
	m_Velocity = DirectX::SimpleMath::Vector3::Zero;
}

bool Enemy::IsChasing() const { return m_State && m_State->IsChaseState(); }
float Enemy::GetSearchSpeed() const { return m_SearchSpeed; }
float Enemy::GetChaseSpeed() const { return m_ChaseSpeed; }
float Enemy::GetChasePathUpdateInterval() const { return m_ChasePathUpdateInterval; }
float Enemy::GetLostStateDuration() const { return m_LostStateDuration; }
DirectX::SimpleMath::Vector3 Enemy::GetRotation() const { return m_Rotation; }
DirectX::SimpleMath::Vector3 Enemy::GetVelocity() const { return m_Velocity; }
void Enemy::SetRotation(const DirectX::SimpleMath::Vector3& rot) { m_Rotation = rot; }
void Enemy::SetCurrentMaxSpeed(float speed) { m_CurrentMaxSpeed = speed; }


// ============================================================
// JSONからパラメータを読み直して即座に反映する
// ============================================================
void Enemy::ReloadParams()
{
	const auto& data = ConfigManager::GetInstance().GetData();
	if (data.is_null()) return;

	try
	{
		// --- Common (基本動作) ---
		if (data.contains("Common"))
		{
			auto& common = data["Common"];
			m_SearchSpeed = common.value("SearchSpeed", m_SearchSpeed);
			m_ChaseSpeed = common.value("ChaseSpeed", m_ChaseSpeed);
			m_MaxAccel = common.value("MaxAccel", m_MaxAccel);

			// JSON上は度数で持っているのでラジアンに変換してから格納する
			if (common.contains("MaxAngVelDeg"))
			{
				float deg = common.value("MaxAngVelDeg", 320.0f);
				m_MaxAngVel = DirectX::XMConvertToRadians(deg);
			}
		}

		// --- Detection (検知・視界) ---
		if (data.contains("Detection"))
		{
			auto& detect = data["Detection"];
			m_DetectionRadius = detect.value("Radius", m_DetectionRadius);
			m_DetectionFOV = detect.value("FOV", m_DetectionFOV);
			m_CatchRange = detect.value("CatchRange", m_CatchRange);
			m_LostStateDuration = detect.value("LostDuration", m_LostStateDuration);
		}

		// --- AI (思考・パス検索) ---
		if (data.contains("AI"))
		{
			auto& ai = data["AI"];
			m_ChasePathUpdateInterval = ai.value("ChasePathUpdateInterval", m_ChasePathUpdateInterval);
			m_PathCornerCutDistance = ai.value("CornerCutDist", m_PathCornerCutDistance);
		}
	}
	catch (std::exception& e)
	{
		(void)e; // Releaseビルドでは未使用になるため警告抑制
#ifdef _DEBUG
		std::cout << "[Enemy::ReloadParams] Json読み込み失敗: " << e.what() << std::endl;
#endif
	}

	// 視野角のコサイン値を再計算（変更が内積判定に反映されるよう毎回計算する）
	m_FOVThreshold = std::cos(DirectX::XMConvertToRadians(m_DetectionFOV * 0.5f));

	// m_CurrentMaxSpeedは状態遷移時にしか更新されないため、ここで再同期しないと
	// 索敵/追跡速度を変更してもその場の速度に反映されない
	m_CurrentMaxSpeed = IsChasing() ? m_ChaseSpeed : m_SearchSpeed;
}

// ============================================================
// 2つの角度の差を -PI 〜 +PI の範囲で返す
// ============================================================
static float AngleDiff(float a, float b)
{
	float d = std::fmod(b - a + DirectX::XM_PI, DirectX::XM_2PI);
	if (d < 0) d += DirectX::XM_2PI;
	return d - DirectX::XM_PI;
}


Enemy::Enemy(const Vector3& pos, const Vector3& size)
	: ColliderObject(pos, size)
{
	m_UID = s_NextUID++;
}

Enemy::~Enemy() {}

// ============================================================
//初期化処理
// ------------------------------------------------------------
void Enemy::Init()
{
	StaticMesh staticmesh;

	std::u8string modelFile = u8"assets/model/enemy/Enemy.fbx";
	std::string texDirectory = "assets/model/enemy";

	std::string tmp(reinterpret_cast<const char*>(modelFile.c_str()), modelFile.size());
	staticmesh.Load(tmp, texDirectory);

	m_MeshRenderer.Init(staticmesh);

	//当たり判定用のサイズを設定
	collider.size = GetScale() * (staticmesh.GetMax() - staticmesh.GetMin());
	m_Position.y += collider.size.y * 0.25f;//モデル側の問題で床にめり込むので少し上げる
	collider.center = m_Position;

	// 壁回避やウィスカー判定に使用するマージンを、敵のサイズに合わせて設定
	float halfX = collider.size.x * 0.5f;
	float halfZ = collider.size.z * 0.5f;
	m_WallMargin = std::sqrt(halfX * halfX + halfZ * halfZ);

	m_ArriveRadius = MAP::Config::BLOCK_SIZE / 3.0f;// チェックポイント到達とみなす距離

	m_subsets = staticmesh.GetSubsets();
	m_Textures = staticmesh.GetTextures();

	auto materials = staticmesh.GetMaterials();
	for (size_t i = 0; i < materials.size(); ++i)
	{
		auto mat = std::make_unique<Material>();
		mat->Create(materials[i]);
		m_Materials.push_back(std::move(mat));
	}

	m_Shader.Create("shader/litTextureVS.hlsl", "shader/litTexturePS.hlsl");

	//プレイヤーオブジェクトへの弱参照を取得
	m_CachedPlayer = SceneManager::GetInstance().FindObject<Player>();

	//最後にプレイヤーがいた位置を初期化
	m_LastPlayerPos = m_Position;

	//スポットライトを追加
	m_SpotLightId = Renderer::AddSpotLight(
		m_Position,
		Vector3::Down,
		m_SpotLightRange,
		m_SpotLightAngleDeg,
		m_SpotLightColor
	);

	// 設定ファイルからパラメータを読み込み
	ReloadParams();

	m_CurrentMaxSpeed = m_SearchSpeed; // 最初の速度を探索速度に設定

	// デフォルトで探索状態から開始
	ChangeState(std::make_unique<EnemySearchState>());
	m_FOVThreshold = std::cos(DirectX::XMConvertToRadians(m_DetectionFOV * 0.5f));// 視野角の半分の余弦値を計算
}




// ============================================================
// Update
// ------------------------------------------------------------
void Enemy::Update(float deltaTime)
{
	UpdateColliderRotation();       //OBB コライダーの回転を自身の向きに同期
	UpdatePlayerCache();            //プレイヤーへの弱参照が無効なら再取得
	UpdateVisitedCells(deltaTime);  //現在グリッドセルを訪問履歴に記録

	//状態マシンを更新
	if (m_State)
	{
		m_State->Update(this, deltaTime);
	}

	CheckForPlayerCollision();  //プレイヤーと触れていたらゲームオーバー
	FollowPath(deltaTime);      //現在のウェイポイントに沿って移動

	// 水平速度に応じて上下移動のボブを計算
	float horizSpeed = sqrtf(m_Velocity.x * m_Velocity.x + m_Velocity.z * m_Velocity.z);
	constexpr float BOB_FREQ = 8.0f;
	constexpr float BOB_AMP = 2.5f;
	float maxSpeed = std::max(m_CurrentMaxSpeed, 1.0f);
	m_BobPhase += horizSpeed * BOB_FREQ * deltaTime / maxSpeed;
	float speedRatio = std::min(horizSpeed / maxSpeed, 1.0f);
	m_BobOffset = sinf(m_BobPhase) * BOB_AMP * speedRatio;


	UpdateRotation(deltaTime);  //目標方向へ滑らかに旋回
	UpdateSpotLight();          //スポットライトを現在の位置・向きに更新
}




// ============================================================
// OBB コライダーの回転を自身の向きに同期
// ============================================================
void Enemy::UpdateColliderRotation()
{
	collider.rotation = DirectX::SimpleMath::Quaternion::CreateFromYawPitchRoll(
		m_Rotation.y, m_Rotation.x, m_Rotation.z);
}


// ============================================================
// プレイヤーへの弱参照キャッシュを管理
// ============================================================
void Enemy::UpdatePlayerCache()
{
	// キャッシュした弱参照が無効になっていたら再度取得を試みる
	if (m_CachedPlayer.expired())
	{
		m_CachedPlayer = SceneManager::GetInstance().FindObject<Player>();
	}
}


// ============================================================
// 現在いるグリッドセルを訪問履歴に記録
// ------------------------------------------------------------
void Enemy::UpdateVisitedCells(float deltaTime)
{
	m_GameTime += deltaTime;

	if (!m_Map) return;

	GridCoord currentCell = Pathfinder::WorldToGrid(m_Map, m_Position);

	// 前回と同じセルなら記録不要
	if (!m_RecentlyVisitedCells.empty() &&
		m_RecentlyVisitedCells.back().coord.x == currentCell.x &&
		m_RecentlyVisitedCells.back().coord.y == currentCell.y)
	{
		return;
	}

	// 新しいセルに入ったので記録する
	RecentVisit visit;
	visit.coord = currentCell;
	visit.timestamp = m_GameTime;
	m_RecentlyVisitedCells.push_back(visit);

	// 履歴サイズを上限以内に収める
	while (m_RecentlyVisitedCells.size() > MAX_RECENT_VISITS)
	{
		m_RecentlyVisitedCells.pop_front();
	}
}


// ============================================================
// 旋回処理
// ------------------------------------------------------------
void Enemy::UpdateRotation(float deltaTime)
{
	const float TARGET_FPS = 60.0f;
	const float dt = 1.0f / TARGET_FPS;
	const float frameScale = deltaTime * TARGET_FPS;

	// 現在の向きと目標の向きの差分を求める
	float diff = AngleDiff(m_Rotation.y, m_TargetYaw);

	//徐々に目標角度に近づける
	float step = m_MaxAngVel * dt * frameScale;
	diff = std::clamp(diff, -step, step);
	m_Rotation.y += diff;
}


// ============================================================
// 敵の頭部に追従するスポットライトを更新
// ------------------------------------------------------------
// フレーム開始時に一度だけ全ライトをクリア
// 敵の正面方向を斜め下に傾けた角度でスポットライトを照射。
// ============================================================
void Enemy::UpdateSpotLight()
{
	// 敵の正面ベクトルを計算
	Vector3 forward;
	forward.x = sin(m_Rotation.y);
	forward.y = 0.0f;
	forward.z = cos(m_Rotation.y);
	forward.Normalize();

	// スポットライトの照射方向（正面から斜め下 55°）
	const float downAngleDeg = 55.0f;
	const float downAngleRad = DirectX::XMConvertToRadians(downAngleDeg);

	Vector3 lightDir;
	lightDir.x = forward.x * cos(downAngleRad);
	lightDir.y = -sin(downAngleRad);
	lightDir.z = forward.z * cos(downAngleRad);
	lightDir.Normalize();

	// ライトの発生位置（敵の頭部付近・少し前方）
	float headHeight = collider.size.y * 0.8f; // コライダー高さの 80% の位置
	Vector3 lightPos = m_Position;
	lightPos.y += headHeight;
	lightPos += forward * 0.3f; // 少し前方にオフセット

	// 有効なスポットライト ID がある場合のみ更新
	if (m_SpotLightId >= 0)
	{
		Renderer::UpdateSpotLight(
			m_SpotLightId,       // 確保済みのスポットライト ID
			lightPos,            // 新しい位置
			lightDir,            // 新しい向き
			m_SpotLightRange,    // 照射範囲
			m_SpotLightAngleDeg, // 照射角度
			m_SpotLightColor     // 色
		);
	}
}



// ============================================================
// 描画処理
// ------------------------------------------------------------
// F1/F6 などのキーで各デバッグ表示を切り替え。
// ============================================================
void Enemy::Draw()
{
	auto camera = Game::GetInstance().GetMainCamera();

	//カリング判定
	if (!camera.GetFrustum().Intersects(collider.ToBoundingBox()))
	{
		return; // 視界に入っていないので処理終了
	}

	Matrix r = Matrix::CreateFromYawPitchRoll(m_Rotation.y + DirectX::XM_PI, m_Rotation.x, m_Rotation.z);
	Matrix s = Matrix::CreateScale(m_Scale.x, m_Scale.y, m_Scale.z);
	Vector3 centerPos = m_Position;

	// コライダーの足元位置を計算
	Vector3 modelFootPos = m_Position;
	modelFootPos.y -= collider.size.y * 0.5f;


	// --- モデルの描画 (足元基準) ---
	modelFootPos.y += m_BobOffset;
	Matrix t_model = Matrix::CreateTranslation(modelFootPos);
	Matrix world = s * r * t_model;
	Renderer::SetWorldMatrix(&world);
	m_Shader.SetGPU();
	m_MeshRenderer.BeforeDraw();

	for (size_t i = 0; i < m_subsets.size(); ++i)
	{
		int matIdx = m_subsets[i].MaterialIdx;
		m_Materials[matIdx]->SetGPU();

		if (m_Materials[matIdx]->isTextureEnable())
		{
			m_Textures[matIdx]->SetGPU();
		}

		m_MeshRenderer.DrawSubset(m_subsets[i].IndexNum,
			m_subsets[i].IndexBase,
			m_subsets[i].VertexBase);
	}

	// デバッグ表示
	if (DebugManager::GetInstance().ShouldShowEnemyWhisker())
	{
		DrawDebugWhiskerLines();
	}

	if (DebugManager::GetInstance().ShouldShowEnemyPath())
	{
		DrawDebugWaypoints();
	}

	if (DebugManager::GetInstance().ShouldShowEnemyVision())
	{
		DrawDebugVision();
	}

	if (DebugManager::GetInstance().ShouldShowColliders())
	{
		collider.DrawDebugCollider(camera);
	}
}

// シャドウマップ描画
void Enemy::DrawShadow()
{
	Matrix r = Matrix::CreateFromYawPitchRoll(m_Rotation.y + DirectX::XM_PI, m_Rotation.x, m_Rotation.z);
	Matrix s = Matrix::CreateScale(m_Scale.x, m_Scale.y, m_Scale.z);

	Vector3 modelFootPos = m_Position;
	modelFootPos.y -= collider.size.y * 0.5f;
	modelFootPos.y += m_BobOffset;

	Matrix t = Matrix::CreateTranslation(modelFootPos);
	Matrix worldmtx = s * r * t;

	Renderer::SetWorldMatrix(&worldmtx);
	Renderer::SetShadowStaticShader();
	m_MeshRenderer.BeforeDraw();

	for (size_t i = 0; i < m_subsets.size(); ++i)
	{
		m_MeshRenderer.DrawSubset(
			m_subsets[i].IndexNum,
			m_subsets[i].IndexBase,
			m_subsets[i].VertexBase);
	}
}

// ============================================================
// Uninit() - 終了処理
// ============================================================
void Enemy::Uninit()
{
	m_State.reset();

	//スポットライトの削除
	if (m_SpotLightId >= 0)
	{
		Renderer::RemoveLight(m_SpotLightId);
		m_SpotLightId = -1; // IDを無効化しておく
	}
}



// ============================================================
// 状態を切り替える
// ------------------------------------------------------------
void Enemy::ChangeState(std::unique_ptr<EnemyState> newState)
{
	if (m_State)
	{
		m_State->Exit(this);
	}

	// 新しい状態の名前を記録して遷移履歴に積む
	// MSVC の typeid().name() は "class EnemyChaseState" のように "class " が付くので除去する
	if (newState)
	{
		std::string raw = typeid(*newState).name();
		const std::string prefix = "class ";
		if (raw.find(prefix) == 0)
			raw = raw.substr(prefix.size());

		auto& entry = EnemyDebugData::Registry()[m_UID];
		entry.history.push_back({ raw, m_GameTime });
		if ((int)entry.history.size() > EnemyDebugData::HISTORY_SIZE)
			entry.history.pop_front();
		entry.currentStateName = raw;
	}

	m_State = std::move(newState);
	if (m_State)
	{
		m_State->Enter(this);
	}
}




// ============================================================
// 指定ターゲットまでの経路を A* で計算する
// ------------------------------------------------------------
// パスが見つかった場合は true、見つからなかった場合は false を返す。
// ============================================================
bool Enemy::ComputePathTo(const Vector3& target)
{
	if (!m_Map) return false;

	//このフレームのパス計算回数制限をチェック
	//1フレームあたりの計算回数を制限
	if (m_PathCalculationCount >= MAX_CALCULATIONS_PER_FRAME) return false;

	m_PathCalculationCount++;


	// 自身の現在位置と目的地をグリッド座標に変換
	GridCoord start = Pathfinder::WorldToGrid(m_Map, m_Position);
	GridCoord goal = Pathfinder::WorldToGrid(m_Map, target);

	//グリッド上のパスを計算
	auto gridPath = Pathfinder::FindPath(m_Map, start, goal);// この時点ではグリッドに沿った角ばったパスが返される

	// パスが見つからなかった場合
	if (gridPath.empty())
	{
		return false;
	}

	// グリッド座標のパスをワールド座標に変換
	std::vector<Vector3> worldPath;
	worldPath.reserve(gridPath.size());

	for (const auto& cell : gridPath)
	{
		worldPath.push_back(Pathfinder::GridToWorld(m_Map, cell));
	}

	// 最終ウェイポイントをグリッドセル中心ではなく実際のターゲット位置で上書き
	if (!worldPath.empty())
	{
		worldPath.back() = target;
	}

	// パスの後処理（スムージング、始点スキップ、コーナーカット）
	ProcessPath(worldPath);

	//ウェイポイントが存在すれば成功
	return !m_Waypoints.empty();
}





// ============================================================
// パスの後処理（スムージング・始点スキップ・コーナーカット）
// ------------------------------------------------------------
// ComputePathTo() から生のグリッドパスを受け取り、以下の 3 段階で整形する:
//   1. ウィスカー判定を使ったスムージング
//   2. 始点スキップ
//   3. コーナーカット
// ============================================================
void Enemy::ProcessPath(const std::vector<Vector3>& worldPath)
{
	// 敵の体の半径（壁との安全距離）
	float radius = m_WallMargin;

	// ウィスカー判定を行うラムダ式
	auto checkWideLineOfSight = [this, radius](const Vector3& start, const Vector3& end) -> bool
		{
			return this->CheckWideLineOfSight(start, end, radius);
		};

	// スムージング実行
	auto smooth = Pathfinder::SmoothPath(worldPath, checkWideLineOfSight);

	// 新しく計算した滑らかなルートを、敵が進む道として設定
	m_Waypoints.clear();
	for (const auto& p : smooth)
	{
		m_Waypoints.push_back(p);
	}

	// 始点スキップ処理
	if (m_Waypoints.size() >= 2)
	{
		Vector3 firstPoint = m_Waypoints[0];
		Vector3 nextPoint = m_Waypoints[1];

		// 現在地から最初のウェイポイントまでの水平距離を計算
		float dx = firstPoint.x - m_Position.x;
		float dz = firstPoint.z - m_Position.z;
		float distXZ = std::sqrt(dx * dx + dz * dz);

		// 閾値を設定
		float threshold = MAP::Config::BLOCK_SIZE * 0.8f;

		if (distXZ < threshold)
		{
			// 視線判定用に少し高さをオフセット
			Vector3 startCheck = m_Position + Vector3(0, 0.5f, 0);
			Vector3 endCheck = nextPoint + Vector3(0, 0.5f, 0);

			// 十分な速度で移動中かつウェイポイントが進行方向の後方にある場合は無条件スキップ
			Vector3 vel2D = m_Velocity;
			vel2D.y = 0.0f;
			Vector3 toFirst = firstPoint - m_Position;
			toFirst.y = 0.0f;
			bool isFirstBehind = (vel2D.LengthSquared() > 25.0f)
				&& (toFirst.Dot(vel2D) < 0.0f);

			// 現在地から2番目のウェイポイントへの見通しがあるか確認 (幅を持った判定で)
			if (isFirstBehind || CheckWideLineOfSight(startCheck, endCheck, radius))
			{
				// 見通しがある場合、最初のウェイポイントをスキップ
				m_Waypoints.pop_front();
			}
		}
	}

	// スキップ後にウェイポイントが1つ以下になった場合の安全チェック
	if (m_Waypoints.size() <= 1)
	{
		return; // 失敗
	}

	// ウェイポイントのコーナーカット処理
	if (m_Waypoints.size() >= 2)
	{
		// i=0: 敵の現在位置を「前の点」としてwaypoints[0]もカット
		// → 追跡状態でも、探索状態と同じ早めに曲がり始める挙動に
		{
			Vector3 prevPoint = m_Position;
			Vector3 currentPoint = m_Waypoints[0];

			// 現在の点から1つ前の点への方向ベクトルを計算
			Vector3 directionToPrev = prevPoint - currentPoint;
			float distanceToPrev = directionToPrev.Length();

			if (distanceToPrev >= 0.1f)
			{
				directionToPrev /= distanceToPrev;// オフセット距離を計算（距離の一定割合か、最大カット距離のどちらか小さい方）

				const float maxOffsetRatio = 0.5f;
				float offsetDistance = std::min(m_PathCornerCutDistance, distanceToPrev * maxOffsetRatio);

				// ウェイポイントがm_ArriveRadius以内に入ってしまわないよう制限
				float newDist = distanceToPrev - offsetDistance;
				if (newDist < m_ArriveRadius * 1.2f)
				{
					offsetDistance = distanceToPrev - m_ArriveRadius * 1.2f;
				}

				if (offsetDistance > 0.0f)
				{
					// 新しい位置候補
					Vector3 newPos = currentPoint + (directionToPrev * offsetDistance);
					GridCoord newCell = Pathfinder::WorldToGrid(m_Map, newPos);

					// newPos が安全かどうかをチェック
					if (m_Map->IsWalkable(newCell.x, newCell.y) &&
						m_Map->GetClearance(newCell.x, newCell.y) >= m_WallMargin)
					{
						m_Waypoints[0] = newPos;// ウェイポイントを前の点の方向にずらす
					}
				}
			}
		}

		// i=1以降: 1つ前のウェイポイントを「前の点」として各コーナーをカット
		// 各ウェイポイントを順番に処理 (最後のポイントは除く)
		for (size_t i = 1; i < m_Waypoints.size() - 1; ++i)
		{
			const Vector3& prevPoint = m_Waypoints[i - 1];
			Vector3 currentPoint = m_Waypoints[i];
			const Vector3& nextPoint = m_Waypoints[i + 1];

			// 現在の点から1つ前の点への方向ベクトルを計算
			Vector3 directionToPrev = prevPoint - currentPoint;
			float distanceToPrev = directionToPrev.Length();

			// 距離が極端に短い場合はスキップ
			if (distanceToPrev < 0.1f)
			{
				continue;
			}

			// 方向ベクトルを正規化
			directionToPrev /= distanceToPrev;

			// オフセット距離を計算
			const float maxOffsetRatio = 0.5f;
			float offsetDistance = std::min(m_PathCornerCutDistance, distanceToPrev * maxOffsetRatio);

			// 新しい位置候補
			Vector3 newPos = currentPoint + (directionToPrev * offsetDistance);

			// newPos が安全かどうかをチェック
			GridCoord newCell = Pathfinder::WorldToGrid(m_Map, newPos);
			if (!m_Map->IsWalkable(newCell.x, newCell.y))
			{
				continue;
			}
			if (m_Map->GetClearance(newCell.x, newCell.y) < m_WallMargin)
			{
				continue;
			}

			// ウェイポイントを前の点の方向にずらす
			m_Waypoints[i] = newPos;
		}
	}

	return;
}




// ============================================================
// 幅を持った視線判定（ウィスカー判定）
// ------------------------------------------------------------
bool Enemy::CheckWideLineOfSight(const DirectX::SimpleMath::Vector3& start, const DirectX::SimpleMath::Vector3& end, float radius) const
{
	if (!m_Map) return false;

	// センター線チェック（壁セルへの直接侵入を高速検出）
	if (!HasLineOfSight(start, end, 0.0f))
		return false;

	// 経路方向を計算
	Vector3 toEnd = end - start;
	toEnd.y = 0.0f;
	const float dist = toEnd.Length();
	if (dist < 1e-4f) return true;
	const Vector3 dir = toEnd / dist;

	// AABB正確距離チェック用ラムダ
	//    ある点(worldPos)から、周囲の壁セルのAABB表面まで最短距離を計算し
	//    radius 未満なら false（危険）を返す
	const float halfCell = MAP::Config::BLOCK_SIZE * 0.5f;
	const float radiusSq = radius * radius;
	const int   searchR = static_cast<int>(std::ceil(radius / MAP::Config::BLOCK_SIZE)) + 1;

	auto isPointSafe = [&](const Vector3& worldPos) -> bool
		{
			GridCoord center = Pathfinder::WorldToGrid(m_Map, worldPos);
			for (int dx = -searchR; dx <= searchR; ++dx)
			{
				for (int dy = -searchR; dy <= searchR; ++dy)
				{
					int cx = center.x + dx;
					int cy = center.y + dy;
					// 境界外はすでにマップ外なので壁として扱う必要なし
					if (cx < 0 || cy < 0 || cx >= m_Map->GetSizeX() || cy >= m_Map->GetSizeY()) continue;
					// 通路セルはスキップ
					if (m_Map->IsWalkable(cx, cy)) continue;

					// 壁セル中心のワールド座標を取得
					Vector3 wallCenter = Pathfinder::GridToWorld(m_Map, { cx, cy });

					// 点からAABBへの最短距離を計算
					float ox = std::max(0.0f, std::abs(worldPos.x - wallCenter.x) - halfCell);
					float oz = std::max(0.0f, std::abs(worldPos.z - wallCenter.z) - halfCell);
					if (ox * ox + oz * oz < radiusSq)
						return false; // 壁に近すぎる
				}
			}
			return true;
		};

	// 経路全体を(BLOCK_SIZE/4)間隔でサンプリングして安全性を確認
	const float sampleStep = MAP::Config::BLOCK_SIZE * 0.25f;
	for (float d = 0.0f; d <= dist; d += sampleStep)
	{
		if (!isPointSafe(start + dir * d))
			return false;
	}
	// エンドポイントも明示的に確認
	if (!isPointSafe(end))
		return false;

	return true;
}




// ============================================================
// ウェイポイントに沿って物理的に移動する
// ------------------------------------------------------------
void Enemy::FollowPath(float deltaTime)
{
	// フレームレート補正係数を計算
	const float TARGET_FPS = 60.0f;
	const float dt = 1.0f / TARGET_FPS;
	const float frameScale = deltaTime * TARGET_FPS; // 実際のフレームレートに応じたスケール


	if (m_Waypoints.empty()) return;

	// 到達していたら次へ
	const Vector3& head = m_Waypoints.front();
	Vector3 toHead = head - m_Position;
	toHead.y = 0.0f;
	float distHead = toHead.Length();

	// 追跡中の最終ウェイポイントはOBBが必ず重なる距離まで詰める
	float useRadius = (m_State && m_State->IsChaseState() && m_Waypoints.size() == 1) ? 2.0f : m_ArriveRadius;
	if (distHead <= useRadius)
	{
		m_Waypoints.pop_front();
		if (m_Waypoints.empty())
		{
			m_Velocity = Vector3::Zero; // 停止
			return;
		}
	}

	// 複数セグメントを結合した拡張パスを構築
	std::vector<Vector3> extendedPath;
	extendedPath.push_back(m_Position); // 現在位置

	int segmentsToLookAhead = std::min(3, (int)m_Waypoints.size());
	auto it = m_Waypoints.begin();
	for (int i = 0; i < segmentsToLookAhead; ++i)
	{
		extendedPath.push_back(*it);
		++it;
	}

	// 拡張パス上で最も近い点を見つける
	Vector3 bestTarget = extendedPath[1];
	bestTarget.y = m_Position.y;
	float bestDist = FLT_MAX;

	for (size_t i = 0; i < extendedPath.size() - 1; ++i)
	{
		const Vector3& segA = extendedPath[i];
		const Vector3& segB = extendedPath[i + 1];

		Vector3 seg = segB - segA;
		float segLen2 = std::max(seg.LengthSquared(), 1e-6f);
		Vector3 toPos = m_Position - segA;

		// 内積を使って、線分上の進行割合 t を求める
		float t = std::clamp(toPos.Dot(seg) / segLen2, 0.0f, 1.0f);
		Vector3 closest = segA + seg * t;// 線上の最も近い点

		// 先読み距離分だけセグメント方向に進む
		Vector3 segDir = seg;
		segDir.Normalize();

		Vector3 lookAheadPoint = closest + segDir * m_LookAhead;// そこから先読み距離分だけ進んだ点をターゲットにする


		Vector3 lookDiff = lookAheadPoint - segA;
		float tLook = std::clamp(lookDiff.Dot(seg) / segLen2, 0.0f, 1.0f);
		Vector3 candidate = segA + seg * tLook;

		float distToCandidate = (candidate - m_Position).Length();
		if (distToCandidate < bestDist)
		{
			bestDist = distToCandidate;
			bestTarget = candidate;
		}

		// 先読み距離に達したら終了
		if (tLook >= 1.0f)
		{
			// 次のセグメントにまたがる可能性がある場合は継続
			float remaining = m_LookAhead - (candidate - closest).Length();
			if (remaining > 0.1f && i < extendedPath.size() - 2)
			{
				continue;
			}
			else
			{
				break;
			}
		}
	}

	Vector3 targetPoint = bestTarget;
	targetPoint.y = m_Position.y; // 高さを強制的に合わせる

	Vector3 desiredDir = targetPoint - m_Position;
	desiredDir.y = 0.0f; // Y成分を完全カット
	float desiredLen = desiredDir.Length();
	if (desiredLen > 1e-5f) desiredDir /= desiredLen;

	Vector3 desiredVel = desiredDir * m_CurrentMaxSpeed;

	GridCoord currentCell = Pathfinder::WorldToGrid(m_Map, m_Position);
	float clearance = m_Map->GetClearance(currentCell.x, currentCell.y);

	// 壁に近づきすぎている場合は速度を落とす
	if (clearance < m_WallMargin * 1.5f && clearance > 0.0f)
	{
		float safetyFactor = clearance / (m_WallMargin * 1.5f);
		safetyFactor = std::clamp(safetyFactor, 0.3f, 1.0f);
		desiredVel *= safetyFactor;
	}


	if (m_Waypoints.size() == 1)
	{
		float slowDownDist = 3.0f;
		float factor = std::clamp(distHead / slowDownDist, 0.15f, 1.0f);
		desiredVel *= factor;
	}

	Vector3 steer = desiredVel - m_Velocity;
	float steerLen = steer.Length();
	float maxStep = m_MaxAccel * dt;
	if (steerLen > maxStep) steer *= (maxStep / steerLen);
	m_Velocity += steer;

	float vlen = m_Velocity.Length();
	if (vlen > m_CurrentMaxSpeed) m_Velocity *= (m_CurrentMaxSpeed / vlen);

	AttemptMovementSlide(m_Velocity * deltaTime);
}


// ============================================================
// 壁スライド移動処理
// ------------------------------------------------------------
// delta 方向への移動を試み、壁と衝突する場合は軸別スライドで回避する。
// ============================================================
void Enemy::AttemptMovementSlide(const Vector3& delta)
{
	if (!m_Map) { m_Position += delta; collider.center = m_Position; return; }

	Vector3 originalPos = m_Position;
	Vector3 targetPos = originalPos + delta;
	float requiredClearance = m_WallMargin * 0.95f;

	// 目標セルが安全なら直接移動
	GridCoord targetCell = Pathfinder::WorldToGrid(m_Map, targetPos);
	if (m_Map->IsWalkable(targetCell.x, targetCell.y))
	{
		if (m_Map->GetClearance(targetCell.x, targetCell.y) >= requiredClearance)
		{
			m_Position = targetPos;
			collider.center = m_Position;
			return;
		}
	}

	// 移動量を段階的に縮小して安全な最大距離を探す
	Vector3 safePos = originalPos;
	if (delta.Length() >= 1e-6f)
	{
		const int steps = 5;
		for (int i = steps; i >= 1; --i)
		{
			Vector3 testPos = originalPos + delta * (static_cast<float>(i) / steps);
			GridCoord testCell = Pathfinder::WorldToGrid(m_Map, testPos);
			if (m_Map->IsWalkable(testCell.x, testCell.y) &&
				m_Map->GetClearance(testCell.x, testCell.y) >= requiredClearance)
			{
				safePos = testPos;
				break;
			}
		}
	}

	// 縮小でも動けない場合は軸別スライドを試みる
	if ((safePos - originalPos).Length() < 1e-3f)
	{
		Vector3 slidePos = originalPos;
		bool movedAny = false;

		if (std::abs(delta.x) > 1e-6f)
		{
			Vector3 xMove = originalPos + Vector3(delta.x * 0.8f, 0, 0);
			GridCoord xCell = Pathfinder::WorldToGrid(m_Map, xMove);
			if (m_Map->IsWalkable(xCell.x, xCell.y) &&
				m_Map->GetClearance(xCell.x, xCell.y) >= requiredClearance)
			{
				slidePos.x = xMove.x;
				movedAny = true;
			}
		}

		if (std::abs(delta.z) > 1e-6f)
		{
			Vector3 zMove = originalPos + Vector3(0, 0, delta.z * 0.8f);
			GridCoord zCell = Pathfinder::WorldToGrid(m_Map, zMove);
			if (m_Map->IsWalkable(zCell.x, zCell.y) &&
				m_Map->GetClearance(zCell.x, zCell.y) >= requiredClearance)
			{
				slidePos.z = zMove.z;
				movedAny = true;
			}
		}

		if (movedAny)
		{
			GridCoord slideCell = Pathfinder::WorldToGrid(m_Map, slidePos);
			// 最終確認時のスレッショルドを緩めすぎると次フレームで抜け出せなくなるため限界値を設定
			if (m_Map->IsWalkable(slideCell.x, slideCell.y) &&
				m_Map->GetClearance(slideCell.x, slideCell.y) >= requiredClearance * 0.9f)
			{
				safePos = slidePos;
			}
		}
	}

	if ((safePos - originalPos).Length() > 1e-3f)
		m_Position = safePos;
	else
		m_Velocity = Vector3::Zero; // どうしても動けない場合は停止

	collider.center = m_Position;
}



// ============================================================
// プレイヤーとの接触判定
// ------------------------------------------------------------
// 透明化中・無敵モード中はスキップ。
// ============================================================
void Enemy::CheckForPlayerCollision()
{
	if (auto player = m_CachedPlayer.lock())
	{
		if (player->IsInvisible())// プレイヤーが透明化中なら接触判定を無効化
		{
			return; // 処理を中断
		}


		// 無敵モード中はプレイヤーとの接触判定をスキップ
		if (DebugManager::GetInstance().IsInvincibleMode())
		{
			return;
		}

		// 自身のコライダーとプレイヤーのコライダーで当たり判定をチェック
		if (this->collider.CheckCollision(player->GetCollider()))
		{
			// 接触したら、ゲームオーバー処理
			player->SetCanMove(false); // プレイヤーの移動を停止
			SceneManager::GetInstance().ChangeScene<GameOverScene>(std::make_unique<FadeTransition>(3.0f));
		}
	}
}




// ============================================================
// プレイヤーが視界内に入っているか判定
// ------------------------------------------------------------
bool Enemy::CanSeePlayer()
{
	if (auto player = m_CachedPlayer.lock())
	{
		// プレイヤーが透明化中なら、絶対に見えない
		if (player->IsInvisible())
		{
			return false;
		}

		Vector3 playerPos = player->GetPosition();

		float heightDiff = std::abs(playerPos.y - m_Position.y);//敵とプレイヤーの高さの差

		// 設定した高さ制限を超えていたら見えない
		if (heightDiff > m_DetectionHeightLimit)
		{
			return false;
		}

		Vector3 toPlayer = playerPos - m_Position;//敵からプレイヤーへのベクトル
		float distance = toPlayer.Length();
		toPlayer.Normalize();

		if (distance > m_DetectionRadius) return false;//視認範囲外ならfalse

		Vector3 forward = Vector3(std::sin(m_Rotation.y), 0.0f, std::cos(m_Rotation.y));// 自分の正面方向のベクトルを計算
		float dot = forward.Dot(toPlayer);//内積を計算

		if (dot < m_FOVThreshold) return false;// 視野外ならfalse

		//プレイヤーとの間に障害物がないかチェック
		if (HasLineOfSight(m_Position, playerPos, 0.0f))
		{
			m_LastPlayerPos = playerPos;
			return true;//見えたらtrue
		}
	}

	return false;
}





// ============================================================
// 次の探索ターゲットセルを選ぶ
// ------------------------------------------------------------
// 未訪問かつ最近通った場所を避けながら、スコアリングで優先度を付けた候補から
// 上位 30% をランダムに1つ選んで ComputePathTo() を呼び出す。
// 全セル訪問済みなら m_Visited をリセットして再スキャン。
// ============================================================
bool Enemy::ChooseNextSearchTarget()
{
	//パス計算の枠が埋まっていたら諦める
	if (m_PathCalculationCount >= MAX_CALCULATIONS_PER_FRAME)
	{
		return false;
	}


	if (!m_Map) return false;

	// 最近訪問したセルのセットを作成（高速検索用）
	std::unordered_set<int> recentlyVisited;
	const float RECENT_TIME_THRESHOLD = 15.0f; // 15秒以内に訪問したセルを避ける

	for (const auto& visit : m_RecentlyVisitedCells)
	{
		if (m_GameTime - visit.timestamp < RECENT_TIME_THRESHOLD)
		{
			int index = visit.coord.x * m_Map->GetSizeY() + visit.coord.y;
			recentlyVisited.insert(index);
		}
	}

	// 候補セルをスコアリングするローカルヘルパー
	struct CandidateCell { GridCoord coord; float score; };

	auto buildCandidates = [&](bool skipVisited) -> std::vector<CandidateCell>
	{
		std::vector<CandidateCell> result;
		for (int x = 0; x < m_Map->GetSizeX(); ++x)
		{
			for (int y = 0; y < m_Map->GetSizeY(); ++y)
			{
				int index = x * m_Map->GetSizeY() + y;
				if (!m_Map->IsWalkable(x, y)) continue;
				if (skipVisited && m_Visited.find(index) != m_Visited.end()) continue;

				CandidateCell candidate;
				candidate.coord = { x, y };
				candidate.score = 100.0f;

				if (recentlyVisited.find(index) != recentlyVisited.end())
					candidate.score -= 80.0f;

				Vector3 candidateWorld = Pathfinder::GridToWorld(m_Map, candidate.coord);
				candidate.score += (candidateWorld - m_Position).Length() * 0.01f;

				result.push_back(candidate);
			}
		}
		return result;
	};

	// 未訪問セルから候補を作成。全訪問済みなら訪問セットをリセットして再スキャン
	std::vector<CandidateCell> candidates = buildCandidates(true);
	if (candidates.empty())
	{
		m_Visited.clear();
		candidates = buildCandidates(false);
		if (candidates.empty()) return false;
	}

	// スコアの高い候補を選択（上位30%からランダム）
	std::sort(candidates.begin(), candidates.end(),
		[](const CandidateCell& a, const CandidateCell& b) {
			return a.score > b.score;
		});

	size_t topCount = std::max(size_t(1), candidates.size() * 3 / 10);
	std::vector<CandidateCell> topCandidates(candidates.begin(), candidates.begin() + topCount);


	// 上位候補からランダムに1セルを選択するための乱数生成器の初期化
	static std::random_device rd;
	static std::mt19937 rng(rd());
	std::uniform_int_distribution<size_t> dist(0, topCandidates.size() - 1);
	GridCoord targetCell = topCandidates[dist(rng)].coord;

	Vector3 worldTarget = Pathfinder::GridToWorld(m_Map, targetCell);

	if (ComputePathTo(worldTarget))
	{
		m_CurrentSearchTarget = targetCell;
		m_Visited.insert(targetCell.x * m_Map->GetSizeY() + targetCell.y);
		return true;
	}
	return false;
}



// 現在の探索ターゲットを訪問済みとしてマーク
void Enemy::MarkTargetVisited()
{
	if (m_CurrentSearchTarget.x >= 0)
	{
		m_Visited.insert(m_CurrentSearchTarget.x * m_Map->GetSizeY() + m_CurrentSearchTarget.y);
		m_CurrentSearchTarget = { -1, -1 };
	}
}





// ============================================================
// 2点間に壁があるかチェックする（クリアランスマップ利用）
// ------------------------------------------------------------
// 固定間隔ではなくクリアランス距離だけ一気にスキップ」する可変ステップ方式。
// margin を指定すると壁からその距離以内に入ったら遮蔽とみなす。
// ゴール直近ではマージンを緩めて、目標地点手前での false 判定を防ぐ。
// ============================================================
bool Enemy::HasLineOfSight(const Vector3& start, const Vector3& target, float margin) const
{
	if (!m_Map) return false;

	Vector3 toTarget = target - start;
	float totalDist = toTarget.Length();

	// 距離がほぼゼロなら見えている
	if (totalDist < 1e-4f) return true;

	Vector3 direction = toTarget / totalDist; // 正規化ベクトル

	float currentDist = 0.0f;
	Vector3 currentPos = start;

	// ループ開始
	// 従来の一定間隔ではなく可変間隔で進む
	while (currentDist < totalDist)
	{
		//現在地のグリッド座標を取得
		GridCoord cell = Pathfinder::WorldToGrid(m_Map, currentPos);

		//マップからここから一番近い壁までの距離をもらう
		float safetyRadius = m_Map->GetClearance(cell.x, cell.y);

		//マージン分を考慮して実際に進める距離を計算
		// 壁までの距離が margin より小さい = マージンを含めると壁にぶつかっている
		if (safetyRadius <= margin)
		{
			// ゴールが直近にある場合だけは許容する
			float distToGoal = totalDist - currentDist;
			if (distToGoal < margin + MAP::Config::BLOCK_SIZE) // ゴールがすぐそば
			{
				return true;
			}
			return false; // 壁に接触した
		}

		// 安全距離分だけ一気にスキップ
		// ただし、クリアランスマップはグリッドセルの中心点から壁までの距離を記録しているため、
		// セル内の任意の点では実際の壁距離が最大でセル半対角線（BLOCK_SIZE * sqrt(2)/2 ≈ 38.9）分
		// だけ過大評価される可能性がある。この誤差を差し引いて保守的なステップ量にする。
		const float cellApproxError = MAP::Config::BLOCK_SIZE * (std::sqrtf(2.0f) / 2.0f);
		float step = std::max(safetyRadius - margin - cellApproxError, MAP::Config::BLOCK_SIZE * 0.1f);

		// ゴールを超えて進まないように制限
		if (currentDist + step > totalDist)
		{
			// ゴール到達遮蔽物なし
			return true;
		}

		//座標更新
		currentPos += direction * step;
		currentDist += step;
	}

	return true;
}



//デバッグ用：ウェイポイントを描画
void Enemy::DrawDebugWaypoints() const
{
	// --- ウェイポイントが空か、デバッグリソースが初期化されていなければ描画しない ---
	if (m_Waypoints.empty() || !SimpleBoxCollider::m_initialized)
	{
		return;
	}

	// --- SimpleBoxCollider から静的リソースを取得 ---
	auto context = Renderer::GetDeviceContext();
	auto& effect = *SimpleBoxCollider::m_Effect;
	auto& batch = *SimpleBoxCollider::m_Batch;
	auto& states = *SimpleBoxCollider::m_States;
	auto  inputLayout = SimpleBoxCollider::m_InputLayout.Get();
	auto pCamera = Game::GetInstance().GetMainCamera();


	// --- 描画ステートの保存 ---
	D3D11DebugState saved{};
	SaveD3D11DebugState(context, saved);


	// --- デバッグ描画の開始 ---
	effect.SetWorld(Matrix::Identity);
	effect.SetView(pCamera.GetViewMatrix());
	effect.SetProjection(pCamera.GetProjectionMatrix());
	effect.Apply(context);

	context->IASetInputLayout(inputLayout);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
	context->OMSetDepthStencilState(states.DepthNone(), 0); // 常に手前に表示

	batch.Begin();

	// --- ここからがウェイポイント描画処理 ---

	// 色の定義
	const XMVECTORF32 COLOR_FIRST_LEG = Colors::LimeGreen; // 敵から最初の点 (緑)
	const XMVECTORF32 COLOR_FUTURE_PATH = Colors::Yellow;  // 経路 (黄)
	const XMVECTORF32 COLOR_POINTS = Colors::Red;          // 点 (赤)

	//敵の現在位置から最初のウェイポイント」への線
	const Vector3& firstWaypoint = m_Waypoints.front();
	batch.DrawLine(
		VertexPositionColor(m_Position, COLOR_FIRST_LEG),
		VertexPositionColor(firstWaypoint, COLOR_FIRST_LEG)
	);

	//各ウェイポイント（点）と、ウェイポイント間の線を描画
	for (size_t i = 0; i < m_Waypoints.size(); ++i)
	{
		const Vector3& wp = m_Waypoints[i];

		// ウェイポイント（点）を十字で描画
		float pointSize = 0.5f; // 点の大きさ（十字の半径）
		batch.DrawLine(VertexPositionColor(wp - Vector3(pointSize, 0, 0), COLOR_POINTS),
			VertexPositionColor(wp + Vector3(pointSize, 0, 0), COLOR_POINTS));
		batch.DrawLine(VertexPositionColor(wp - Vector3(0, pointSize, 0), COLOR_POINTS),
			VertexPositionColor(wp + Vector3(0, pointSize, 0), COLOR_POINTS));
		batch.DrawLine(VertexPositionColor(wp - Vector3(0, 0, pointSize), COLOR_POINTS),
			VertexPositionColor(wp + Vector3(0, 0, pointSize), COLOR_POINTS));


		//ウェイポイント間の線を描画 (i が最後の要素でなければ)
		if (i < m_Waypoints.size() - 1)
		{
			const Vector3& wp_next = m_Waypoints[i + 1];
			batch.DrawLine(
				VertexPositionColor(wp, COLOR_FUTURE_PATH),
				VertexPositionColor(wp_next, COLOR_FUTURE_PATH)
			);
		}
	}
	// --- ウェイポイント描画ここまで ---

	batch.End();


	// --- 描画ステートの復元 ---
	RestoreD3D11DebugState(context, saved);
}


// デバッグ用：敵の視界を描画
void Enemy::DrawDebugVision() const
{
	// --- デバッグリソースが初期化されていなければ描画しない ---
	if (!SimpleBoxCollider::m_initialized)
	{
		return;
	}

	// --- SimpleBoxCollider から静的リソースを取得 ---
	auto context = Renderer::GetDeviceContext();
	auto& effect = *SimpleBoxCollider::m_Effect;
	auto& batch = *SimpleBoxCollider::m_Batch;
	auto& states = *SimpleBoxCollider::m_States;
	auto  inputLayout = SimpleBoxCollider::m_InputLayout.Get();
	auto pCamera = Game::GetInstance().GetMainCamera();

	// --- 描画ステートの保存 ---
	D3D11DebugState saved{};
	SaveD3D11DebugState(context, saved);

	// --- デバッグ描画の開始 ---
	// ワールド座標系で描画
	effect.SetWorld(Matrix::Identity);
	effect.SetView(pCamera.GetViewMatrix());
	effect.SetProjection(pCamera.GetProjectionMatrix());
	effect.Apply(context);

	context->IASetInputLayout(inputLayout);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	// 常に手前に表示（壁に隠れないように）
	context->OMSetDepthStencilState(states.DepthNone(), 0);

	batch.Begin();


	// 視界の色（白）
	const XMVECTORF32 COLOR_VISION = Colors::White;

	// 敵の正面の向き (Y軸回転ラジアン)
	const float enemyYaw = m_Rotation.y;

	// 視野角の半分 (ラジアン)
	const float halfFOVRads = DirectX::XMConvertToRadians(m_DetectionFOV * 0.5f);

	// 扇形の開始角度と終了角度 (Y軸回転ラジアン)
	const float startAngle = enemyYaw - halfFOVRads;
	const float endAngle = enemyYaw + halfFOVRads;

	//扇形の辺 (左)
	Vector3 leftEdge;
	leftEdge.x = m_Position.x + m_DetectionRadius * std::sin(startAngle);
	leftEdge.y = m_Position.y;
	leftEdge.z = m_Position.z + m_DetectionRadius * std::cos(startAngle);

	batch.DrawLine(
		VertexPositionColor(m_Position, COLOR_VISION),
		VertexPositionColor(leftEdge, COLOR_VISION)
	);

	//扇形の辺 (右)
	Vector3 rightEdge;
	rightEdge.x = m_Position.x + m_DetectionRadius * std::sin(endAngle);
	rightEdge.y = m_Position.y;
	rightEdge.z = m_Position.z + m_DetectionRadius * std::cos(endAngle);

	batch.DrawLine(
		VertexPositionColor(m_Position, COLOR_VISION),
		VertexPositionColor(rightEdge, COLOR_VISION)
	);

	//扇形の円弧部分 (10セグメントに分割して描画)
	const int arcSegments = 10;
	Vector3 prevPoint = leftEdge; // 最初の点は左端

	for (int i = 1; i <= arcSegments; ++i)
	{
		// t は 0.0 から 1.0 まで変化
		float t = static_cast<float>(i) / static_cast<float>(arcSegments);
		float currentAngle = (1.0f - t) * startAngle + t * endAngle;

		Vector3 currentPoint;
		currentPoint.x = m_Position.x + m_DetectionRadius * std::sin(currentAngle);
		currentPoint.y = m_Position.y;
		currentPoint.z = m_Position.z + m_DetectionRadius * std::cos(currentAngle);

		batch.DrawLine(
			VertexPositionColor(prevPoint, COLOR_VISION),
			VertexPositionColor(currentPoint, COLOR_VISION)
		);

		prevPoint = currentPoint; // 次の線分のために点を保存
	}


	batch.End();


	// --- 描画ステートの復元 ---
	RestoreD3D11DebugState(context, saved);
}

void Enemy::DrawDebugWhiskerLines()
{
	if (!SimpleBoxCollider::m_initialized) return;

	// --- SimpleBoxCollider から静的リソースを取得 ---
	auto context = Renderer::GetDeviceContext();
	auto& effect = *SimpleBoxCollider::m_Effect;
	auto& batch = *SimpleBoxCollider::m_Batch;
	auto& states = *SimpleBoxCollider::m_States;
	auto  inputLayout = SimpleBoxCollider::m_InputLayout.Get();
	auto pCamera = Game::GetInstance().GetMainCamera();

	// --- 描画ステートの保存 ---
	D3D11DebugState saved{};
	SaveD3D11DebugState(context, saved);

	// --- デバッグ描画の開始 ---
	effect.SetWorld(Matrix::Identity);
	effect.SetView(pCamera.GetViewMatrix());
	effect.SetProjection(pCamera.GetProjectionMatrix());
	effect.Apply(context);

	context->IASetInputLayout(inputLayout);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	// 常に手前に表示
	context->OMSetDepthStencilState(states.DepthNone(), 0);

	batch.Begin();

	// --- ウィスカー線の描画ロジック ---
	// CheckWideLineOfSight と同じ幅・方向で描画（判定と可視化を一致させる）
	float lineLength = 30.0f;
	float r = m_WallMargin; // 判定と同じ幅
	float diagR = r * 0.707f; // 斜め方向のオフセット

	float yaw = m_Rotation.y;
	Vector3 forward = Vector3(std::sin(yaw), 0.0f, std::cos(yaw));

	Vector3 startPos = m_Position;
	startPos.y += 0.5f;

	Color colorAxis = DirectX::SimpleMath::Color(DirectX::Colors::Purple);
	Color colorDiag = DirectX::SimpleMath::Color(DirectX::Colors::Magenta);

	// 8方向のオフセット（CheckWideLineOfSight と同じ）
	const Vector3 offsets[8] = {
		{ 0.0f, 0.0f, 0.0f },           // 中央
		{ r,    0.0f, 0.0f },            // +X
		{-r,    0.0f, 0.0f },            // -X
		{ 0.0f, 0.0f, r    },            // +Z
		{ 0.0f, 0.0f,-r    },            // -Z
		{ diagR, 0.0f, diagR },          // +X+Z
		{ diagR, 0.0f,-diagR },          // +X-Z
		{-diagR, 0.0f, diagR },          // -X+Z
	};

	for (int i = 0; i < 8; ++i)
	{
		Vector3 s = startPos + offsets[i];
		Vector3 e = s + forward * lineLength;
		Color c = (i == 0) ? colorAxis : (i <= 4 ? colorAxis : colorDiag);
		batch.DrawLine(VertexPositionColor(s, c), VertexPositionColor(e, c));
	}

	batch.End();

	// --- 描画ステートの復元 ---
	RestoreD3D11DebugState(context, saved);
}





