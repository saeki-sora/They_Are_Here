#include "pch.h"
#include "Enemy.h"
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

using namespace DirectX;
using namespace DirectX::SimpleMath;


using namespace DirectX::SimpleMath;

int Enemy::m_PathCalculationCount = 0;

static float AngleDiff(float a, float b)
{
	float d = std::fmod(b - a + DirectX::XM_PI, DirectX::XM_2PI);
	if (d < 0) d += DirectX::XM_2PI;
	return d - DirectX::XM_PI;
}



//今フレームのパス計算数をリセット
void Enemy::ResetPathCalculationCount() { m_PathCalculationCount = 0; }


Enemy::Enemy(const Vector3& pos, const Vector3& size)
	: ColliderObject(pos, size)
{
}

Enemy::~Enemy() {}

void Enemy::Init()
{
	StaticMesh staticmesh;

	std::u8string modelFile = u8"assets/model/enemy/Enemy_Mid.fbx";
	std::string texDirectory = "assets/model/enemy";

	std::string tmp(reinterpret_cast<const char*>(modelFile.c_str()), modelFile.size());
	staticmesh.Load(tmp, texDirectory);

	m_MeshRenderer.Init(staticmesh);

	//当たり判定用のサイズを設定
	collider.size = GetScale() * (staticmesh.GetMax() - staticmesh.GetMin());
	m_Position.y += collider.size.y * 0.25f;//モデル側の問題で床にめり込むので少し上げる
	collider.center = m_Position;

	m_ArriveRadius = MAP::Config::BLOCK_SIZE / 3.0f;// チェックポイント到達とみなす距離

	m_subsets = staticmesh.GetSubsets();
	m_Textures = staticmesh.GetTextures();

	auto materials = staticmesh.GetMaterials();
	for (size_t i = 0; i < materials.size(); ++i)
	{
		auto mat = std::make_unique<Material>();
		mat->Create(materials[i]);
		m_Materiales.push_back(std::move(mat));
	}

	m_Shader.Create("shader/litTextureVS.hlsl", "shader/litTexturePS.hlsl");

	//プレイヤーオブジェクトへの弱参照を取得
	m_CachedPlayer = SceneManager::GetInstance().FindObject<Player>();

	//最後にプレイヤーがいた位置を初期化
	m_LastPlayerPos = m_Position;

	//スポットライトを追加
	m_SpotLightId = Renderer::AddSpotLight(
		m_Position,
		Vector3::Down, // 仮の下向き
		m_SpotLightRange,
		m_SpotLightAngleDeg,
		m_SpotLightColor
	);

	// 設定ファイルからパラメータを読み込み
	const auto& data = ConfigManager::GetInstance().GetData();
	if (!data.is_null())
	{
		try
		{
			// --- Common (基本動作) ---
			if (data.contains("Common"))
			{
				auto& common = data["Common"];
				m_SearchSpeed = common.value("SearchSpeed", m_SearchSpeed);
				m_ChaseSpeed = common.value("ChaseSpeed", m_ChaseSpeed);
				m_MaxAccel = common.value("MaxAccel", m_MaxAccel);

				// 旋回速度
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
				m_PredictionTime = ai.value("PredictionTime", m_PredictionTime);
				m_PathCornerCutDistance = ai.value("CornerCutDist", m_PathCornerCutDistance);
			}
		}
		catch (std::exception& e)
		{
			std::cout << "[Enemy::Init] Json読み込み失敗" << e.what() << std::endl;
		}
	}

	m_WallMargin = std::max(collider.size.x, collider.size.z) * 0.5f;

	m_CurrentMaxSpeed = m_SearchSpeed; // 最初の速度を探索速度に設定

	// デフォルトで探索状態から開始
	ChangeState(std::make_unique<EnemySearchState>());
	m_FOVThreshold = std::cos(DirectX::XMConvertToRadians(m_DetectionFOV * 0.5f));// 視野角の半分の余弦値を計算
}




void Enemy::Update(float deltaTime)
{
	// もしキャッシュが空なら再度取得を試みる
	if (m_CachedPlayer.expired())
	{
		m_CachedPlayer = SceneManager::GetInstance().FindObject<Player>();
	}

	m_GameTime += deltaTime;

	// 現在のグリッド位置を記録
	if (m_Map)
	{
		GridCoord currentCell = Pathfinder::WorldToGrid(m_Map, m_Position);

		// 最新の訪問記録と異なるセルに移動した場合のみ記録
		if (m_RecentlyVisitedCells.empty() ||
			m_RecentlyVisitedCells.back().coord.x != currentCell.x ||
			m_RecentlyVisitedCells.back().coord.y != currentCell.y)
		{
			RecentVisit visit;
			visit.coord = currentCell;
			visit.timestamp = m_GameTime;
			m_RecentlyVisitedCells.push_back(visit);

			// 履歴サイズを制限
			while (m_RecentlyVisitedCells.size() > MAX_RECENT_VISITS)
			{
				m_RecentlyVisitedCells.pop_front();
			}
		}
	}

	// 状態マシンの更新
	if (m_State)
	{
		m_State->Update(this, deltaTime);
	}

	// プレイヤーとの接触判定
	CheckForPlayerCollision();

	FollowPath(deltaTime);// パスに沿って移動

	// 旋回処理
	const float TARGET_FPS = 60.0f;
	const float dt = 1.0f / TARGET_FPS;
	const float frameScale = deltaTime * TARGET_FPS;

	// m_TargetYaw は m_State->Update()によって設定
	float diff = AngleDiff(m_Rotation.y, m_TargetYaw);

	// 直接追跡モードの時だけ旋回を速くする
	float currentMaxAngVel = m_UseDirectChase ? (m_MaxAngVel * 2.0f) : m_MaxAngVel;

	// 1フレームで回転できる角度に制限をかけて徐々に目標角度に近づける
	float step = currentMaxAngVel * dt * frameScale;
	diff = std::clamp(diff, -step, step);
	m_Rotation.y += diff;


	// フレーム開始時に一度だけライトをクリア
	static bool lightsCleared = false;
	if (!lightsCleared)
	{
		Renderer::ClearLights();
		lightsCleared = true;
	}

	// 敵の正面ベクトルを計算（Y成分は常に0にする）
	Vector3 forward;
	forward.x = sin(m_Rotation.y);
	forward.y = 0.0f;
	forward.z = cos(m_Rotation.y);
	forward.Normalize();

	// スポットライトの方向を計算（正面から下向きに45度）
	const float downAngleDeg = 55.0f; // 下向きの角度（度）
	const float downAngleRad = DirectX::XMConvertToRadians(downAngleDeg);


	Vector3 lightDir;
	lightDir.x = forward.x * cos(downAngleRad);
	lightDir.y = -sin(downAngleRad);
	lightDir.z = forward.z * cos(downAngleRad);
	lightDir.Normalize();

	// ライトの位置（敵の頭部付近）
	float headHeight = collider.size.y * 0.8f; // コライダーの高さの80%の位置
	Vector3 lightPos = m_Position;
	lightPos.y += headHeight;
	lightPos += forward * 0.3f; // 少し前方にオフセット

	// スポットライトの情報を更新
	if (m_SpotLightId >= 0) // IDが有効な場合のみ
	{
		Renderer::UpdateSpotLight(
			m_SpotLightId,       // 確保済みのID
			lightPos,            // 新しい位置
			lightDir,            // 新しい向き
			m_SpotLightRange,    // 範囲
			m_SpotLightAngleDeg, // 角度
			m_SpotLightColor     // 色
		);
	}

	// デバッグ出力（60フレームに1回）
	//static int debugTimer = 0;
	//if (debugTimer++ % 60 == 0)
	//{
	//	std::cout << "[Enemy] Light ID: " << m_SpotLightId
	//		<< " | Pos: " << lightPos.x << ", " << lightPos.y << ", " << lightPos.z
	//		<< " | Dir: " << lightDir.x << ", " << lightDir.y << ", " << lightDir.z
	//		<< "\n";
	//}
}



void Enemy::Draw()
{
	auto camera = Game::GetInstance().GetMainCamera();

	//カリング判定
	if (!camera.GetFrustum().Intersects(collider.ToBoundingBox()))
	{
		return; // 視界に入っていないので、ここで処理終了
	}

	Matrix r = Matrix::CreateFromYawPitchRoll(m_Rotation.y + DirectX::XM_PI, m_Rotation.x, m_Rotation.z);
	Matrix s = Matrix::CreateScale(m_Scale.x, m_Scale.y, m_Scale.z);
	Vector3 centerPos = m_Position;

	// コライダーの足元位置を計算
	Vector3 modelFootPos = m_Position;
	modelFootPos.y -= collider.size.y * 0.5f;


	// --- モデルの描画 (足元基準) ---
	Matrix t_model = Matrix::CreateTranslation(modelFootPos);
	Matrix world = s * r * t_model;
	Renderer::SetWorldMatrix(&world);
	m_Shader.SetGPU();
	m_MeshRenderer.BeforeDraw();

	for (size_t i = 0; i < m_subsets.size(); ++i)
	{
		int matIdx = m_subsets[i].MaterialIdx;
		m_Materiales[matIdx]->SetGPU();

		if (m_Materiales[matIdx]->isTextureEnable())
		{
			m_Textures[matIdx]->SetGPU();
		}

		m_MeshRenderer.DrawSubset(m_subsets[i].IndexNum,
			m_subsets[i].IndexBase,
			m_subsets[i].VertexBase);
	}

	if (DebugManager::GetInstance().ShouldShowEnemyPath())
	{
		DrawDebugWaypoints();
	}

	if (DebugManager::GetInstance().ShouldShowEnemyVision())
	{
		DrawDebugVision();
	}
}

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



void Enemy::ChangeState(std::unique_ptr<EnemyState> newState)
{
	if (m_State)
	{
		m_State->Exit(this);
	}
	m_State = std::move(newState);
	if (m_State)
	{
		m_State->Enter(this);
	}
}





// 目的地までのパスを計算する関数
// この関数は、A*アルゴリズムを使ってグリッド上のパスを計算し、
// ProcessPath関数を呼び出してスムージングなどの後処理を行う。
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

	// パスの後処理（スムージング、始点スキップ、コーナーカット）
	ProcessPath(worldPath);

	//ウェイポイントが存在すれば成功
	return !m_Waypoints.empty();
}





// 1. ウィスカー判定を使ったスムージング（壁との接触を避けながらパスを直線化）
// 2. 始点スキップ（現在地に近すぎる最初のウェイポイントを削除）
// 3. コーナーカット（曲がり角を滑らかにするため、各点を前の点の方向にオフセット）
void Enemy::ProcessPath(const std::vector<Vector3>& worldPath)
{
	// 敵の体の半径（壁との安全距離）
	float radius = m_WallMargin;

	// ウィスカー判定を行うラムダ式
	auto checkWideLineOfSight = [this, radius](const Vector3& start, const Vector3& end) -> bool
		{
			// まず中心線が通るかチェック
			if (!this->HasLineOfSight(start, end, 0.0f)) return false;

			// 進行方向に対して垂直なベクトルを計算
			Vector3 forward = end - start;
			forward.y = 0.0f; // 水平方向のみ考慮
			if (forward.LengthSquared() < 1e-4f) return true; // 移動距離が短すぎる場合はOK
			forward.Normalize();

			Vector3 right = forward.Cross(Vector3::Up); // 上ベクトルとの外積で右方向を算出

			// オフセット量
			Vector3 offset = right * radius;

			if (!this->HasLineOfSight(start - offset, end - offset, 0.0f)) return false;// 左端の線チェック
			if (!this->HasLineOfSight(start + offset, end + offset, 0.0f)) return false;// 右端の線チェック

			// 3本すべての線が通れば合格
			return true;
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

			// 現在地から2番目のウェイポイントへの見通しがあるか確認
			if (HasLineOfSight(startCheck, endCheck, 0.0f))
			{
				// 見通しがある場合、最初のウェイポイントをスキップ
				m_Waypoints.pop_front();
			}
		}
	}

	// スキップ後にウェイポイントが1つ以下になった場合の安全チェック
	if (m_Waypoints.size() <= 1)
	{
		return ; // 失敗
	}

	// ウェイポイントのコーナーカット処理
	if (m_Waypoints.size() >= 2)
	{
		// 各ウェイポイントを順番に処理
		for (size_t i = 1; i < m_Waypoints.size(); ++i)
		{
			const Vector3& prevPoint = m_Waypoints[i - 1];
			Vector3 currentPoint = m_Waypoints[i];

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

			// ウェイポイントを前の点の方向にずらす
			m_Waypoints[i] = currentPoint + (directionToPrev * offsetDistance);
		}
	}

	//コーナーカット後もウェイポイントが十分にあるかチェック
	if (m_Waypoints.size() <= 1)
	{
		// 警告メッセージ
		// std::cout << "[WARNING] 警告: コーナーカット後、ウェイポイントが少なすぎます\n";
	}

	return;
}




void Enemy::FollowPath(float deltaTime)
{
	// フレームレート補正係数を計算（60FPSを基準とする）
	const float TARGET_FPS = 60.0f;
	const float dt = 1.0f / TARGET_FPS;
	const float frameScale = deltaTime * TARGET_FPS; // 実際のフレームレートに応じたスケール

	// 直接追跡モードのチェック
	if (m_UseDirectChase)
	{
		// 直接追跡モード：プレイヤーの方向に直接向かう
		Vector3 toTarget = m_DirectChaseTarget - m_Position;
		float distance = toTarget.Length();

		if (distance < 1e-5f)
		{
			m_Velocity = Vector3::Zero;
			return;
		}

		toTarget.Normalize();


		// 目標速度を計算
		Vector3 desiredVel = toTarget * m_CurrentMaxSpeed;

		// 加速度で速度を更新
		Vector3 steer = desiredVel - m_Velocity;
		float steerLen = steer.Length();
		float maxStep = m_MaxAccel * dt;
		if (steerLen > maxStep) steer *= (maxStep / steerLen);
		m_Velocity += steer;

		// 最高速クランプ
		float vlen = m_Velocity.Length();
		if (vlen > m_CurrentMaxSpeed) m_Velocity *= (m_CurrentMaxSpeed / vlen);

		// 位置更新
		auto tryMoveSlide = [this](const Vector3& delta)
			{
				if (!m_Map) {
					m_Position += delta;
					collider.center = m_Position;
					return;
				}

				Vector3 originalPos = m_Position;
				Vector3 targetPos = originalPos + delta;

				// === ステップ1: 目標位置の安全性を確認 ===
				GridCoord targetCell = Pathfinder::WorldToGrid(m_Map, targetPos);

				if (m_Map->IsWalkable(targetCell.x, targetCell.y))
				{
					float targetClearance = m_Map->GetClearance(targetCell.x, targetCell.y);

					// 十分なクリアランスがあれば直接移動
					if (targetClearance >= m_WallMargin * 1.2f)
					{
						m_Position = targetPos;
						collider.center = m_Position;
						return;
					}
				}

				// === ステップ2: 移動ベクトルを縮小して再試行 ===
				// 壁に近すぎる場合、移動量を減らして安全な範囲内に収める
				Vector3 safePos = originalPos;
				float deltaLength = delta.Length();

				if (deltaLength < 1e-6f)
				{
					return; // 移動量がほぼゼロなら何もしない
				}

				Vector3 deltaDir = delta / deltaLength;

				// 段階的に移動距離を短くしながら安全な位置を探す
				const int steps = 5;
				for (int i = steps; i >= 1; --i)
				{
					float ratio = static_cast<float>(i) / static_cast<float>(steps);
					Vector3 testPos = originalPos + delta * ratio;
					GridCoord testCell = Pathfinder::WorldToGrid(m_Map, testPos);

					if (m_Map->IsWalkable(testCell.x, testCell.y))
					{
						float testClearance = m_Map->GetClearance(testCell.x, testCell.y);

						if (testClearance >= m_WallMargin * 0.9f)
						{
							safePos = testPos;
							break;
						}
					}
				}

				// === ステップ3: 軸別スライド処理 ===
				// 直接移動できない場合のみ、軸別に試行
				if ((safePos - originalPos).Length() < 1e-3f)
				{
					Vector3 slidePos = originalPos;
					bool movedAny = false;

					// X軸方向の移動を試行
					if (std::abs(delta.x) > 1e-6f)
					{
						Vector3 xOnlyMove = originalPos + Vector3(delta.x * 0.8f, 0, 0);
						GridCoord xCell = Pathfinder::WorldToGrid(m_Map, xOnlyMove);

						if (m_Map->IsWalkable(xCell.x, xCell.y))
						{
							float xClearance = m_Map->GetClearance(xCell.x, xCell.y);
							if (xClearance >= m_WallMargin)
							{
								slidePos.x = xOnlyMove.x;
								movedAny = true;
							}
						}
					}

					// Z軸方向の移動を試行
					if (std::abs(delta.z) > 1e-6f)
					{
						Vector3 zOnlyMove = originalPos + Vector3(0, 0, delta.z * 0.8f);
						GridCoord zCell = Pathfinder::WorldToGrid(m_Map, zOnlyMove);

						if (m_Map->IsWalkable(zCell.x, zCell.y))
						{
							float zClearance = m_Map->GetClearance(zCell.x, zCell.y);
							if (zClearance >= m_WallMargin)
							{
								slidePos.z = zOnlyMove.z;
								movedAny = true;
							}
						}
					}

					// スライド移動が可能だった場合、その位置の最終確認
					if (movedAny)
					{
						GridCoord slideCell = Pathfinder::WorldToGrid(m_Map, slidePos);
						if (m_Map->IsWalkable(slideCell.x, slideCell.y))
						{
							float slideClearance = m_Map->GetClearance(slideCell.x, slideCell.y);
							if (slideClearance >= m_WallMargin * 0.7f)
							{
								safePos = slidePos;
							}
						}
					}
				}

				// === ステップ4: 位置を更新 ===
				if ((safePos - originalPos).Length() > 1e-3f)
				{
					m_Position = safePos;
				}
				else
				{
					// どうしても動けない場合は速度をゼロに
					m_Velocity = Vector3::Zero;
				}

				collider.center = m_Position;
			};

		tryMoveSlide(m_Velocity * dt * frameScale);

		return;
	}


	if (m_Waypoints.empty()) return;

	// ----- セグメント終端処理：到達していたら次へ -----
	const Vector3& head = m_Waypoints.front();
	Vector3 toHead = head - m_Position;
	toHead.y = 0.0f;
	float distHead = toHead.Length();

	if (distHead <= m_ArriveRadius)
	{
		//std::cout << "【移動】ウェイポイント到達！ 次へ進みます。\n";
		m_Waypoints.pop_front();
		if (m_Waypoints.empty())
		{
			//std::cout << "【移動】全ウェイポイント消化完了。停止します。\n";
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

	auto tryMoveSlide = [this](const Vector3& delta)
		{
			if (!m_Map) { m_Position += delta; collider.center = m_Position; return; }

			Vector3 newPos = m_Position;

			if (std::abs(delta.x) > 1e-6f)
			{
				Vector3 probe = newPos + Vector3(delta.x, 0, 0);
				GridCoord c = Pathfinder::WorldToGrid(m_Map, probe);
				if (m_Map->IsWalkable(c.x, c.y))
					newPos.x = probe.x;
			}

			if (std::abs(delta.z) > 1e-6f)
			{
				Vector3 probe = newPos + Vector3(0, 0, delta.z);
				GridCoord c = Pathfinder::WorldToGrid(m_Map, probe);
				if (m_Map->IsWalkable(c.x, c.y))
					newPos.z = probe.z;
			}

			m_Position = newPos;
			collider.center = m_Position;
		};

	tryMoveSlide(m_Velocity * deltaTime);

}



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
			player->CanMove(false); // プレイヤーの移動を停止
			SceneManager::GetInstance().ChangeScene<GameOverScene>(std::make_unique<FadeTransition>(3.0f));
		}
	}
}




bool Enemy::CanSeePlayer()
{
	if (auto player = m_CachedPlayer.lock())
	{
		// プレイヤーがパワーアップ中なら、絶対に見えない
		if (player->IsInvisible())
		{
			return false;
		}

		Vector3 playerPos = player->GetPosition();

		float heightDiff = std::abs(playerPos.y - m_Position.y);

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
			//std::cout << "!!! プレイヤーみつけた !!!\n";
			return true;//見えたらtrue
		}
	}

	return false;
}





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
			int index = visit.coord.x * MAP::Config::MaxY + visit.coord.y;
			recentlyVisited.insert(index);
		}
	}

	// 候補セルをスコアリング
	struct CandidateCell
	{
		GridCoord coord;
		float score;
	};

	std::vector<CandidateCell> candidates;

	for (int x = 0; x < MAP::Config::MaxX; ++x)
	{
		for (int y = 0; y < MAP::Config::MaxY; ++y)
		{
			int index = x * MAP::Config::MaxY + y;

			if (!m_Map->IsWalkable(x, y)) continue;
			if (m_Visited.find(index) != m_Visited.end()) continue;

			CandidateCell candidate;
			candidate.coord = { x, y };
			candidate.score = 100.0f; // 基本スコア

			// 最近訪問したセルはスコアを大幅に減少
			if (recentlyVisited.find(index) != recentlyVisited.end())
			{
				candidate.score -= 80.0f;
			}

			// 現在位置から遠いセルを優先（探索の多様性向上）
			Vector3 candidateWorld = Pathfinder::GridToWorld(m_Map, candidate.coord);
			float distanceFromCurrent = (candidateWorld - m_Position).Length();
			candidate.score += distanceFromCurrent * 0.01f;

			candidates.push_back(candidate);
		}
	}

	// 全セル訪問済みなら訪問セットをクリア
	if (candidates.empty())
	{
		m_Visited.clear();

		for (int x = 0; x < MAP::Config::MaxX; ++x)
		{
			for (int y = 0; y < MAP::Config::MaxY; ++y)
			{
				int index = x * MAP::Config::MaxY + y;
				if (!m_Map->IsWalkable(x, y)) continue;

				CandidateCell candidate;
				candidate.coord = { x, y };
				candidate.score = 100.0f;

				if (recentlyVisited.find(index) != recentlyVisited.end())
				{
					candidate.score -= 80.0f;
				}

				Vector3 candidateWorld = Pathfinder::GridToWorld(m_Map, candidate.coord);
				float distanceFromCurrent = (candidateWorld - m_Position).Length();
				candidate.score += distanceFromCurrent * 0.01f;

				candidates.push_back(candidate);
			}
		}

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
		m_Visited.insert(targetCell.x * MAP::Config::MaxY + targetCell.y);
		return true;
	}
	return false;
}



// 現在の探索ターゲットを訪問済みとしてマーク
void Enemy::MarkTargetVisited()
{
	if (m_CurrentSearchTarget.x >= 0)
	{
		m_Visited.insert(m_CurrentSearchTarget.x * MAP::Config::MaxY + m_CurrentSearchTarget.y);
		m_CurrentSearchTarget = { -1, -1 };
	}
}


// プレイヤーの位置と速度の履歴を更新
void Enemy::UpdatePlayerTracking(float deltaTime)
{
	if (auto player = m_CachedPlayer.lock())
	{
		if (player->IsInvisible())
		{
			return; // 透明化中は追跡しない
		}

		Vector3 currentPos = player->GetPosition();
		Vector3 currentVel = Vector3::Zero;

		// 前回の位置との差分から速度を計算
		if (!m_PlayerHistory.empty() && deltaTime > 0.0001f)
		{
			const auto& lastData = m_PlayerHistory.back();
			currentVel = (currentPos - lastData.position) / deltaTime;
		}

		// 履歴に追加
		PlayerTrackingData newData;
		newData.position = currentPos;
		newData.velocity = currentVel;
		newData.timestamp = 0.0f; // 簡易的にデルタタイムで管理

		m_PlayerHistory.push_back(newData);

		// 履歴サイズを制限
		while (m_PlayerHistory.size() > TRACKING_HISTORY_SIZE)
		{
			m_PlayerHistory.pop_front();
		}
	}
}



// プレイヤーの将来位置を予測
DirectX::SimpleMath::Vector3 Enemy::PredictPlayerPosition() const
{
	if (auto player = m_CachedPlayer.lock())
	{
		Vector3 currentPos = player->GetPosition();

		// プレイヤーまでの距離をチェック
		float distanceToPlayer = (currentPos - m_Position).Length();

		// 距離が範囲外なら予測を使わない
		if (distanceToPlayer < m_MinPredictionDistance ||
			distanceToPlayer > m_MaxPredictionDistance)
		{
			return currentPos;
		}

		// 履歴が不十分なら予測しない
		if (m_PlayerHistory.size() < 3)
		{
			return currentPos;
		}

		// 平均速度を計算（最新の数フレームから）
		Vector3 avgVelocity = Vector3::Zero;
		int sampleCount = std::min(3, (int)m_PlayerHistory.size());

		for (size_t i = m_PlayerHistory.size() - sampleCount; i < m_PlayerHistory.size(); ++i)
		{
			avgVelocity += m_PlayerHistory[i].velocity;
		}
		avgVelocity /= (float)sampleCount;

		// 速度が小さすぎる場合は予測しない
		if (avgVelocity.Length() < 5.0f)
		{
			return currentPos;
		}

		// 予測位置を計算
		Vector3 predictedPos = currentPos + avgVelocity * m_PredictionTime;

		// 予測位置が壁の中にある場合は補正
		if (m_Map)
		{
			GridCoord predictedGrid = Pathfinder::WorldToGrid(m_Map, predictedPos);

			if (!m_Map->IsWalkable(predictedGrid.x, predictedGrid.y))
			{
				// 壁の中なら、現在位置と予測位置の中間点を試す
				Vector3 midPoint = (currentPos + predictedPos) * 0.5f;
				GridCoord midGrid = Pathfinder::WorldToGrid(m_Map, midPoint);

				if (m_Map->IsWalkable(midGrid.x, midGrid.y))
				{
					return midPoint;
				}
				else
				{
					// それでも壁なら予測を使わない
					return currentPos;
				}
			}
		}

		return predictedPos;
	}

	return m_LastPlayerPos;
}




//プレイヤーがどんな動きをしているかを解析し、安定しているかどうかを判定
bool Enemy::IsPlayerMovementStable() const
{
	// 履歴が不十分な場合は不安定と判断
	if (m_PlayerHistory.size() < 4)
	{
		return false;
	}

	// 最新の速度と一つ前の速度を比較
	const auto& latest = m_PlayerHistory[m_PlayerHistory.size() - 1];
	const auto& previous = m_PlayerHistory[m_PlayerHistory.size() - 2];

	// 速度変化量をチェック
	float speedChange = std::abs(latest.velocity.Length() - previous.velocity.Length());
	if (speedChange > m_PlayerSpeedChangeThreshold)
	{
		return false; // 速度が急変した
	}

	// 方向変化をチェック（両方の速度がゼロでない場合のみ）
	float latestSpeed = latest.velocity.Length();
	float previousSpeed = previous.velocity.Length();

	if (latestSpeed > 5.0f && previousSpeed > 5.0f)
	{
		Vector3 latestDir = latest.velocity;
		latestDir.Normalize();
		Vector3 previousDir = previous.velocity;
		previousDir.Normalize();

		float directionDot = latestDir.Dot(previousDir);

		if (directionDot < m_PlayerDirectionChangeThreshold)
		{
			return false; // 方向が急変した
		}
	}

	// 過去数フレームの方向の一貫性をチェック
	if (m_PlayerHistory.size() >= 4)
	{
		Vector3 avgDirection = Vector3::Zero;
		int validSamples = 0;

		for (size_t i = m_PlayerHistory.size() - 3; i < m_PlayerHistory.size(); ++i)
		{
			if (m_PlayerHistory[i].velocity.Length() > 5.0f)
			{
				Vector3 dir = m_PlayerHistory[i].velocity;
				dir.Normalize();
				avgDirection += dir;
				validSamples++;
			}
		}

		if (validSamples > 0)
		{
			avgDirection /= (float)validSamples;
			avgDirection.Normalize();

			// 最新の方向が平均方向と大きく異なる場合は不安定
			if (latestSpeed > 5.0f)
			{
				Vector3 latestDir = latest.velocity;
				latestDir.Normalize();

				if (avgDirection.Dot(latestDir) < m_PlayerDirectionChangeThreshold)
				{
					return false;
				}
			}
		}
	}

	return true; // すべてのチェックをパスしたら安定
}




// 直接追跡モードを使用すべきか判定
bool Enemy::ShouldUseDirectChase() const
{
	if (auto player = m_CachedPlayer.lock())
	{
		if (player->IsInvisible())
		{
			return false;
		}

		Vector3 playerPos = player->GetPosition();
		float distanceToPlayer = (playerPos - m_Position).Length();

		// プレイヤーが一定距離以内にいて、直線的に見える場合のみ直接追跡を使用
		// 距離閾値は4グリッド分程度に設定
		const float directChaseDistance = MAP::Config::BLOCK_SIZE * 4.0f;

		if (distanceToPlayer <= directChaseDistance)
		{
			// プレイヤーまでの見通しをチェック（余白なし）
			if (HasLineOfSight(m_Position, playerPos, 0.0f))
			{
				return true;
			}
		}
	}

	return false;
}



// 直接追跡モード用のターゲット位置を更新
void Enemy::UpdateDirectChase()
{
	if (auto player = m_CachedPlayer.lock())
	{
		m_DirectChaseTarget = player->GetPosition();
	}
}




// 現在のパス更新間隔を取得
float Enemy::GetCurrentPathUpdateInterval() const
{
	if (m_IsPlayerMovementStable)// プレイヤーの動きが安定している場合
	{
		return m_StablePathUpdateInterval; // 1.5秒
	}
	else// 不安定な場合
	{
		return m_UnstablePathUpdateInterval; // 0.15秒
	}
}



// スタートからターゲットへの直線上に障害物がないかチェック
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
	// 従来の「一定間隔」ではなく「可変間隔」で進みます
	while (currentDist < totalDist)
	{
		// 1. 現在地のグリッド座標を取得
		GridCoord cell = Pathfinder::WorldToGrid(m_Map, currentPos);

		// 2. マップから「ここから一番近い壁までの距離」をもらう
		float safetyRadius = m_Map->GetClearance(cell.x, cell.y);

		// 3. マージン分を考慮して「実際に進める距離」を計算
		// 壁までの距離が margin より小さい = マージンを含めると壁にぶつかっている
		if (safetyRadius <= margin)
		{
			// ゴールが直近にある場合だけは許容する（これがないとゴール直前でfalseになる）
			float distToGoal = totalDist - currentDist;
			if (distToGoal < margin + MAP::Config::BLOCK_SIZE) // ゴールがすぐそば
			{
				return true;
			}
			return false; // 壁（またはマージン領域）に接触した
		}

		// 4. 「安全距離」分だけ一気にスキップ！
		// ただし、無限ループ防止のため最小移動量を設定（ブロックの1/10程度）
		float step = std::max(safetyRadius - margin, MAP::Config::BLOCK_SIZE * 0.1f);

		// ゴールを超えて進まないように制限
		if (currentDist + step > totalDist)
		{
			// ゴール到達！遮蔽物なし
			return true;
		}

		// 5. 座標更新
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


	// --- 描画ステートの完全な保存 (SimpleBoxCollider::DrawDebugCollider と同じ) ---
	ID3D11RasterizerState* prevRasterState = nullptr;
	ID3D11DepthStencilState* prevDepthState = nullptr;
	UINT                     prevStencilRef = 0;
	ID3D11BlendState* prevBlendState = nullptr;
	float                    prevBlendFactor[4] = { 0.0f };
	UINT                     prevSampleMask = 0;
	ID3D11InputLayout* prevInputLayout = nullptr;
	D3D11_PRIMITIVE_TOPOLOGY prevTopology;
	ID3D11VertexShader* prevVS = nullptr;
	ID3D11ClassInstance* prevVSClassInstances[256] = { nullptr };
	UINT                     numVSClassInstances = 256;
	ID3D11PixelShader* prevPS = nullptr;
	ID3D11ClassInstance* prevPSClassInstances[256] = { nullptr };
	UINT                     numPSClassInstances = 256;
	ID3D11Buffer* prevVB[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT] = {};
	UINT prevStride[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT] = {};
	UINT prevOffset[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT] = {};
	ID3D11Buffer* prevIB = nullptr;
	DXGI_FORMAT              prevIBFmt = DXGI_FORMAT_UNKNOWN;
	UINT                     prevIBOfs = 0;
	ID3D11Buffer* prevVSCB[4] = {};
	ID3D11Buffer* prevPSCB[4] = {};
	ID3D11ShaderResourceView* prevPSRV[4] = {};
	ID3D11SamplerState* prevPSSamp[4] = {};

	context->RSGetState(&prevRasterState);
	context->OMGetDepthStencilState(&prevDepthState, &prevStencilRef);
	context->OMGetBlendState(&prevBlendState, prevBlendFactor, &prevSampleMask);
	context->IAGetInputLayout(&prevInputLayout);
	context->IAGetPrimitiveTopology(&prevTopology);
	context->VSGetShader(&prevVS, prevVSClassInstances, &numVSClassInstances);
	context->PSGetShader(&prevPS, prevPSClassInstances, &numPSClassInstances);
	context->IAGetVertexBuffers(0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT, prevVB, prevStride, prevOffset);
	context->IAGetIndexBuffer(&prevIB, &prevIBFmt, &prevIBOfs);
	context->VSGetConstantBuffers(0, 4, prevVSCB);
	context->PSGetConstantBuffers(0, 4, prevPSCB);
	context->PSGetShaderResources(0, 4, prevPSRV);
	context->PSGetSamplers(0, 4, prevPSSamp);
	// --- 保存ここまで ---


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

	//敵の現在位置から「最初のウェイポイント」への線
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


	// --- 描画ステートの復元---
	context->IASetVertexBuffers(0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT, prevVB, prevStride, prevOffset);
	context->IASetIndexBuffer(prevIB, prevIBFmt, prevIBOfs);
	for (auto* b : prevVB) if (b) b->Release();
	if (prevIB) prevIB->Release();

	context->VSSetConstantBuffers(0, 4, prevVSCB);
	context->PSSetConstantBuffers(0, 4, prevPSCB);
	for (auto* b : prevVSCB) if (b) b->Release();
	for (auto* b : prevPSCB) if (b) b->Release();

	context->PSSetShaderResources(0, 4, prevPSRV);
	context->PSSetSamplers(0, 4, prevPSSamp);
	for (auto* v : prevPSRV)   if (v) v->Release();
	for (auto* s : prevPSSamp) if (s) s->Release();

	context->RSSetState(prevRasterState);
	context->OMSetDepthStencilState(prevDepthState, prevStencilRef);
	context->OMSetBlendState(prevBlendState, prevBlendFactor, prevSampleMask);
	context->IASetInputLayout(prevInputLayout);
	context->IASetPrimitiveTopology(prevTopology);
	context->VSSetShader(prevVS, prevVSClassInstances, numVSClassInstances);
	context->PSSetShader(prevPS, prevPSClassInstances, numPSClassInstances);

	if (prevRasterState) prevRasterState->Release();
	if (prevDepthState) prevDepthState->Release();
	if (prevBlendState) prevBlendState->Release();
	if (prevInputLayout) prevInputLayout->Release();
	if (prevVS) prevVS->Release();
	for (UINT i = 0; i < numVSClassInstances; ++i) { if (prevVSClassInstances[i]) prevVSClassInstances[i]->Release(); }
	if (prevPS) prevPS->Release();
	for (UINT i = 0; i < numPSClassInstances; ++i) { if (prevPSClassInstances[i]) prevPSClassInstances[i]->Release(); }
	// --- 復元ここまで ---
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
	ID3D11RasterizerState* prevRasterState = nullptr;
	ID3D11DepthStencilState* prevDepthState = nullptr;
	UINT                     prevStencilRef = 0;
	ID3D11BlendState* prevBlendState = nullptr;
	float                    prevBlendFactor[4] = { 0.0f };
	UINT                     prevSampleMask = 0;
	ID3D11InputLayout* prevInputLayout = nullptr;
	D3D11_PRIMITIVE_TOPOLOGY prevTopology;
	ID3D11VertexShader* prevVS = nullptr;
	ID3D11ClassInstance* prevVSClassInstances[256] = { nullptr };
	UINT                     numVSClassInstances = 256;
	ID3D11PixelShader* prevPS = nullptr;
	ID3D11ClassInstance* prevPSClassInstances[256] = { nullptr };
	UINT                     numPSClassInstances = 256;
	ID3D11Buffer* prevVB[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT] = {};
	UINT prevStride[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT] = {};
	UINT prevOffset[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT] = {};
	ID3D11Buffer* prevIB = nullptr;
	DXGI_FORMAT              prevIBFmt = DXGI_FORMAT_UNKNOWN;
	UINT                     prevIBOfs = 0;
	ID3D11Buffer* prevVSCB[4] = {};
	ID3D11Buffer* prevPSCB[4] = {};
	ID3D11ShaderResourceView* prevPSRV[4] = {};
	ID3D11SamplerState* prevPSSamp[4] = {};

	context->RSGetState(&prevRasterState);
	context->OMGetDepthStencilState(&prevDepthState, &prevStencilRef);
	context->OMGetBlendState(&prevBlendState, prevBlendFactor, &prevSampleMask);
	context->IAGetInputLayout(&prevInputLayout);
	context->IAGetPrimitiveTopology(&prevTopology);
	context->VSGetShader(&prevVS, prevVSClassInstances, &numVSClassInstances);
	context->PSGetShader(&prevPS, prevPSClassInstances, &numPSClassInstances);
	context->IAGetVertexBuffers(0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT, prevVB, prevStride, prevOffset);
	context->IAGetIndexBuffer(&prevIB, &prevIBFmt, &prevIBOfs);
	context->VSGetConstantBuffers(0, 4, prevVSCB);
	context->PSGetConstantBuffers(0, 4, prevPSCB);
	context->PSGetShaderResources(0, 4, prevPSRV);
	context->PSGetSamplers(0, 4, prevPSSamp);


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


	//描画ステート復元
	context->IASetVertexBuffers(0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT, prevVB, prevStride, prevOffset);
	context->IASetIndexBuffer(prevIB, prevIBFmt, prevIBOfs);
	for (auto* b : prevVB) if (b) b->Release();
	if (prevIB) prevIB->Release();

	context->VSSetConstantBuffers(0, 4, prevVSCB);
	context->PSSetConstantBuffers(0, 4, prevPSCB);
	for (auto* b : prevVSCB) if (b) b->Release();
	for (auto* b : prevPSCB) if (b) b->Release();

	context->PSSetShaderResources(0, 4, prevPSRV);
	context->PSSetSamplers(0, 4, prevPSSamp);
	for (auto* v : prevPSRV)   if (v) v->Release();
	for (auto* s : prevPSSamp) if (s) s->Release();
	context->RSSetState(prevRasterState);
	context->OMSetDepthStencilState(prevDepthState, prevStencilRef);
	context->OMSetBlendState(prevBlendState, prevBlendFactor, prevSampleMask);
	context->IASetInputLayout(prevInputLayout);
	context->IASetPrimitiveTopology(prevTopology);
	context->VSSetShader(prevVS, prevVSClassInstances, numVSClassInstances);
	context->PSSetShader(prevPS, prevPSClassInstances, numPSClassInstances);

	if (prevRasterState) prevRasterState->Release();
	if (prevDepthState) prevDepthState->Release();
	if (prevBlendState) prevBlendState->Release();
	if (prevInputLayout) prevInputLayout->Release();
	if (prevVS) prevVS->Release();
	for (UINT i = 0; i < numVSClassInstances; ++i) { if (prevVSClassInstances[i]) prevVSClassInstances[i]->Release(); }
	if (prevPS) prevPS->Release();
	for (UINT i = 0; i < numPSClassInstances; ++i) { if (prevPSClassInstances[i]) prevPSClassInstances[i]->Release(); }
}

