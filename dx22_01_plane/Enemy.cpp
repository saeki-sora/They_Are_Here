#include "Enemy.h"
#include "Game.h"
#include "MakeMap.h"
#include "Player.h"
#include "Renderer.h"
#include "StaticMesh.h"
#include "utility.h"
#include "EnemyState_Search.h"

#include <random>
#include <cmath>
#include <SimpleMath.h>

#define NOMINMAX

using namespace DirectX::SimpleMath;

Enemy::Enemy(const Vector3& pos, const Vector3& size)
	: ColliderObject(pos, size)
{
}

Enemy::~Enemy() {}

void Enemy::Init()
{

	StaticMesh staticmesh;

	std::u8string modelFile = u8"assets/model/player/Player.fbx";
	std::string texDirectory = "assets/texture/terain.png";

	std::string tmp(reinterpret_cast<const char*>(modelFile.c_str()), modelFile.size());
	staticmesh.Load(tmp, texDirectory);

	m_MeshRenderer.Init(staticmesh);

	//当たり判定用のサイズを設定
	//collider.size = GetScale() * (staticmesh.GetMax() - staticmesh.GetMin());

	m_subsets = staticmesh.GetSubsets();
	m_Textures = staticmesh.GetTextures();

	auto materials = staticmesh.GetMaterials();
	for (int i = 0; i < materials.size(); ++i)
	{
		auto mat = std::make_unique<Material>();
		mat->Create(materials[i]);
		m_Materiales.push_back(std::move(mat));
	}

	m_Shader.Create("shader/litTextureVS.hlsl", "shader/litTexturePS.hlsl");

	//最初だけとりあえず自分の位置を最後にプレイヤーを見た場所とする
	m_LastPlayerPos = m_Position;

	// デフォルトで探索状態から開始
	ChangeState(std::make_unique<EnemySearchState>());

	m_FOVThreshold = std::cos(DirectX::XMConvertToRadians(80.0f * 0.5f));//80が視認範囲の半径
}

void Enemy::Update()
{
	constexpr float dt = 1.0f / 60.0f;

	if (m_State)
	{
		m_State->Update(this, dt);
	}

	FollowPath();// パスに沿って移動
}

void Enemy::Draw()
{
	// PlayerやBlockと同様の標準的なレンダリング
	Matrix r = Matrix::CreateFromYawPitchRoll(m_Rotation.y, m_Rotation.x, m_Rotation.z);
	Matrix t = Matrix::CreateTranslation(m_Position.x, m_Position.y, m_Position.z);
	Matrix s = Matrix::CreateScale(m_Scale.x, m_Scale.y, m_Scale.z);
	Matrix world = s * r * t;
	Renderer::SetWorldMatrix(&world);
	m_Shader.SetGPU();
	m_MeshRenderer.BeforeDraw();
	for (int i = 0; i < m_subsets.size(); ++i)
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

#ifdef _DEBUG
	Matrix colliderWorldMatrix = r * t;
	collider.DrawDebugCollider(Game::GetInstance().GetMainCamera(), colliderWorldMatrix);
#endif
}

void Enemy::Uninit()
{
	m_State.reset();
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

bool Enemy::ComputePathTo(const Vector3& target)
{
	if (!m_Map) return false;

	//自身の現在位置と目的地をグリッド座標に変換
	GridCoord start = Pathfinder::WorldToGrid(m_Map, m_Position);
	GridCoord goal = Pathfinder::WorldToGrid(m_Map, target);


	//現在位置から目的地までのパスを計算
	auto gridPath = Pathfinder::FindPath(m_Map, start, goal);
	if (gridPath.empty())
	{
		std::cout << "!!! PATH NOT FOUND !!!\n";
		return false;
	}

	//パスをワールド座標に変換
	std::vector<Vector3> worldPath;
	worldPath.reserve(gridPath.size());
	for (const auto& cell : gridPath)
	{
		worldPath.push_back(Pathfinder::GridToWorld(m_Map, cell));
	}


	// 見通しチェックを使用して滑らかなパスを生成
	auto smooth = Pathfinder::SmoothPath(worldPath, [this](const Vector3& a, const Vector3& b)
		{
			return this->HasLineOfSight(a, b);
		});

	// 新しく計算した滑らかなルートを自分が進む道として設定
	m_Waypoints.clear();
	for (const auto& p : smooth)
	{
		m_Waypoints.push_back(p);
	}

	std::cout << "Path Found! Waypoints:\n";
	for (const auto& wp : m_Waypoints)
	{
		GridCoord grid_wp = Pathfinder::WorldToGrid(m_Map, wp);
		std::cout << "  -> World(" << wp.x << ", " << wp.z << ") Grid(" << grid_wp.x << ", " << grid_wp.y << ")\n";
	}
	std::cout << "--------------------------\n\n";

	return !m_Waypoints.empty();//１つ以上のウェイポイントがあるならtrue
}

void Enemy::FollowPath()
{
	if (m_Waypoints.empty()) { return; }

	// 現在のウェイポイントに向かって移動
	const Vector3& target = m_Waypoints.front();
	Vector3 toTarget = target - m_Position;
	float distance = toTarget.Length();

	if (distance < 1.0f)
	{

		std::cout << ">>> Reached Waypoint: (" << target.x << ", " << target.z << ") <<<\n";

		//このウェイポイントを削除し、次に進む
		m_Waypoints.pop_front();
		if (!m_Waypoints.empty())
		{
			std::cout << ">>> PATH COMPLETE! <<<\n";
			return;
		}
	}
	else
	{
		// 正規化してウェイポイントに向かって移動
		toTarget.Normalize();
		m_Position += toTarget * m_MoveSpeed;
		collider.center = m_Position;
		// 移動方向を向くように前進ベクトルを更新
		m_Rotation.y = std::atan2(toTarget.x, toTarget.z);
	}
}

bool Enemy::CanSeePlayer()
{
	auto playerWeak = Game::GetInstance().FindObject<Player>();

	if (auto player = playerWeak.lock())
	{
		Vector3 playerPos = player->GetPosition();
		Vector3 toPlayer = playerPos - m_Position;//敵からプレイヤーへのベクトル

		float distance = toPlayer.Length();
		toPlayer.Normalize();

		if (distance > m_DetectionRadius) return false;//視認範囲外ならfalse

		Vector3 forward = Vector3(std::sin(m_Rotation.y), 0.0f, std::cos(m_Rotation.y));// 自分の正面方向のベクトルを計算
		float dot = forward.Dot(toPlayer);//内積を計算(どれだけ同じ向きを向いているか)

		if (dot < m_FOVThreshold) return false;// 視野外ならfalse

		//プレイヤーとの間に障害物がないかチェック
		if (HasLineOfSight(m_Position, playerPos))
		{
			m_LastPlayerPos = playerPos;
			std::cout << "!!! プレイヤーみつけた !!!\n";
			return true;//見えたらtrue
		}
	}

	return false;
}

bool Enemy::ChooseNextSearchTarget()
{
	if (!m_Map) return false;

	// まだ探索していない歩行可能なセルのリストを作成
	std::vector<GridCoord> candidates;
	for (int x = 0; x < MAP::Config::MaxX; ++x)
	{
		for (int y = 0; y < MAP::Config::MaxY; ++y)
		{
			int index = x * MAP::Config::MaxY + y;

			if (!m_Map->IsWalkable(x, y)) continue;//壁ならスキップ
			if (m_Visited.find(index) != m_Visited.end()) continue;// 既に訪問済みのセルはスキップ

			candidates.push_back({ x, y });
		}
	}

	// もし全セル訪問済みなら訪問セットをクリアして再探索
	if (candidates.empty())
	{
		m_Visited.clear();

		for (int x = 0; x < MAP::Config::MaxX; ++x)
		{
			for (int y = 0; y < MAP::Config::MaxY; ++y)
			{
				if (!m_Map->IsWalkable(x, y)) continue;
				candidates.push_back({ x, y });
			}
		}

		if (candidates.empty()) return false;
	}

	// ランダムな候補を選択
	static std::random_device rd;
	static std::mt19937 rng(rd());
	std::uniform_int_distribution<size_t> dist(0, candidates.size() - 1);
	GridCoord targetCell = candidates[dist(rng)];

	//ランダムに選んだマスのワールド座標を取得
	Vector3 worldTarget = Pathfinder::GridToWorld(m_Map, targetCell);

	std::cout << "===== New Target Decided =====\n";
	std::cout << "Target Grid: (" << targetCell.x << ", " << targetCell.y << ")\n";
	std::cout << "Target World: (" << worldTarget.x << ", " << worldTarget.y << ", " << worldTarget.z << ")\n";

	//そのマスまでのパスを計算
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



bool Enemy::HasLineOfSight(const Vector3& a, const Vector3& b) const
{
	if (!m_Map) return false;

	Vector3 dir = b - a; float length = dir.Length();

	int samples = static_cast<int>(length / (MAP::Config::BLOCK_SIZE * 0.5f));

	if (samples < 1) return true; dir /= static_cast<float>(samples); 

	Vector3 probe = a; 

	for (int i = 0; i <= samples; ++i) 
	{
		GridCoord cell = Pathfinder::WorldToGrid(m_Map, probe);

		if (!m_Map->IsWalkable(cell.x, cell.y)) { return false; }

		probe += dir;
	} 

	return true;
}

//#undef min
//#undef max
//
//
//bool Enemy::HasLineOfSight(const Vector3& a, const Vector3& b, float margin) const
//{
//	if (!m_Map) return false;
//
//	Vector3 ab = b - a;
//	float   len = ab.Length();
//
//	if (len < 1e-4f) return true;
//
//	// サンプリング間隔をやや細かく
//	const float step = MAP::Config::BLOCK_SIZE * 0.33f;
//
//	int samples = std::max(1, (int)std::ceil(len / step));
//	Vector3 dir = ab / (float)samples;
//
//	// 余白を「何セル分」見るか
//	int marginCells = std::max(0, (int)std::ceil(margin / MAP::Config::BLOCK_SIZE));
//
//	Vector3 p = a;
//	for (int i = 0; i <= samples; ++i)
//	{
//		GridCoord c = Pathfinder::WorldToGrid(m_Map, p);
//
//		// 余白分の近傍セルも壁なら遮蔽扱い
//		for (int dx = -marginCells; dx <= marginCells; ++dx)
//			for (int dy = -marginCells; dy <= marginCells; ++dy)
//			{
//				int nx = c.x + dx;
//				int ny = c.y + dy;
//				if (nx < 0 || ny < 0 || nx >= (int)MAP::Config::MaxX || ny >= (int)MAP::Config::MaxY)
//					return false; // 境界外＝通れない
//				if (!m_Map->IsWalkable(nx, ny))
//					return false;
//			}
//
//		p += dir;
//	}
//	return true;
//}