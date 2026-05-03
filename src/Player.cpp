#include "pch.h"
#include "Player.h"
#include "StaticMesh.h"
#include "utility.h"
#include "Game.h"
#include"Block.h"
#include"Pole.h"
#include"SceneManager.h"
#include "MakeMap.h"
#include "GameClearScene.h"
#include "FadeTransition.h"
#include "TitleScene.h"
#include "EffectManager.h"
#include "InvisibleEffect.h"
#include "InvisibleItem.h"
#include "DebugManager.h"
#include "DashEffect.h"
#include "MiniMap.h"


using namespace std;
using namespace DirectX::SimpleMath;

//コンストラクタ
Player::Player(const Vector3& pos, const Vector3& size)
	: ColliderObject(pos, size),
	m_MouseSensitivity(0.003f),
	m_MoveSpeed(4.5f),
	m_MouseCaptured(true),
	m_Forward(Vector3::UnitZ),
	m_Pitch(0.0f),
	m_InvisibleStock(3),
	m_IsInvisible(false),
	m_InvisibleTimer(0.0f),
	m_InvisibleTime(10.0f)
{
#ifndef _DEBUG

	m_MoveSpeed = 2.0f;

#endif
}


Player::~Player() {}


//=======================================
// 初期化
//=======================================
void Player::Init()
{
	// スタティックメッシュオブジェクトの生成
	StaticMesh staticmesh;

	// モデルファイル
	std::u8string modelFile = u8"assets/model/player/Player.fbx";

	// テクスチャディレクトリ
	std::string texDirectory = "assets/texture/terain.png";

	// Meshを読み込む
	std::string tmpStr1(reinterpret_cast<const char*>(modelFile.c_str()), modelFile.size());
	staticmesh.Load(tmpStr1, texDirectory);

	m_MeshRenderer.Init(staticmesh);

	//当たり判定用のサイズを設定
	collider.size = GetScale() * (staticmesh.GetMax() - staticmesh.GetMin());

	// シェーダオブジェクト生成
	m_Shader.Create("shader/litTextureVS.hlsl", "shader/litTexturePS.hlsl");

	// サブセット情報取得
	m_subsets = staticmesh.GetSubsets();

	// テクスチャ情報取得
	m_Textures = staticmesh.GetTextures();

	// マテリアル情報取得
	vector<MATERIAL> materials = staticmesh.GetMaterials();

	// マテリアル数取得
	for (int i = 0; i < materials.size(); ++i)
	{
		// マテリアルオブジェクト生成
		std::unique_ptr<Material> m = std::make_unique<Material>();

		// マテリアル情報を設定
		m->Create(materials[i]);

		// マテリアルオブジェクトを配列に追加
		m_Materiales.push_back(std::move(m));
	}
}


//========================================
//当たり判定チェック
//========================================
bool Player::CheckCollisionWithBlocks(const Vector3& newPosition)
{

	if (DebugManager::GetInstance().IsNoClipMode())
	{
		return false;// ノークリッピングモードなら衝突しない
	}

	// 新しい位置でのコライダーを作成
	SimpleBoxCollider testCollider = collider;
	testCollider.center = newPosition;

	// ゲームシーン内のすべてのBlockオブジェクトをチェック
	auto blocks = SceneManager::GetInstance().FindAllObjects<Block>();
	for (auto& weakBlock : blocks)
	{
		// weak_ptrをshared_ptrに変換してチェック
		if (auto block = weakBlock.lock())
		{
			// 当たり判定をチェック
			if (testCollider.CheckCollision(block->GetCollider()))
			{
				return true; // 衝突している
			}
		}
	}
	return false; // 衝突していない
}




//=======================================
// 更新処理
//=======================================
void Player::Update(float deltaTime)
{

	collider.rotation = DirectX::SimpleMath::Quaternion::Identity;
	collider.center = m_Position; // コライダーの中心位置を更新

	// Fキーで透明化発動
	if (Input::GetKeyTrigger('F'))
	{
		if (!m_IsInvisible && m_InvisibleStock > 0)
		{
			m_InvisibleStock--; // 減らすだけ。表示は関知しない
			StartInvisible(10.0f);

			EffectManager::GetInstance().StartEffect<InvisibleEffect>(m_InvisibleTime);//透明化エフェクト開始
		}
	}


	//透明化タイマー処理
	if (m_IsInvisible)
	{
		m_InvisibleTimer -= deltaTime;

		if (m_InvisibleTimer <= 0.0f)
		{
			m_IsInvisible = false;
			m_InvisibleTimer = 0.0f;
			std::cout << "!!! POWER UP EXPIRED !!!\n";
		}
	}

	// 移動可能フラグが立っている場合のみ処理を行う
	if (!m_CanMove) return;


	// 壁破壊チャージ処理
	bool currentButtonState = Input::GetKeyPress(VK_LBUTTON);

	if (currentButtonState)
	{
		if (m_WallBreakJustCompleted)
		{
			// 壁破壊直後は押しっぱなしでは再チャージしない
		}
		else if (!m_IsChargingBreak)
		{
			m_TargetBlock = GetBlockInFront();

			if (auto block = m_TargetBlock.lock())
			{
				m_IsChargingBreak = true;
				m_BreakChargeTimer = 0.0f;
				m_ChargeStartYaw = m_Rotation.y;
				m_ChargeStartPitch = m_Pitch;
			}
		}
		else
		{
			// チャージ継続中
			m_BreakChargeTimer += deltaTime;

			if (m_BreakChargeTimer >= WALL_BREAK_CHARGE_TIME)
			{
				TryBreakWall();
				m_IsChargingBreak = false;
				m_BreakChargeTimer = 0.0f;
				m_WallBreakJustCompleted = true;
				m_TargetBlock.reset();
			}
		}
	}
	else
	{
		if (m_IsChargingBreak)
		{
			m_IsChargingBreak = false;
			m_BreakChargeTimer = 0.0f;
			m_TargetBlock.reset();
		}

		m_WallBreakJustCompleted = false;
	}

	// マウスキャプチャ状態時のみマウス操作を処理
	if (m_MouseCaptured)
	{
		int mouseX, mouseY;
		Input::GetMouseMove(&mouseX, &mouseY);

		if (m_IsChargingBreak) // 破壊チャージ中は視点回転を制限
		{
			float desiredYaw = m_Rotation.y + mouseX * m_MouseSensitivity;
			float yawOffset = desiredYaw - m_ChargeStartYaw;

			while (yawOffset > DirectX::XM_PI) yawOffset -= DirectX::XM_2PI;
			while (yawOffset < -DirectX::XM_PI) yawOffset += DirectX::XM_2PI;

			yawOffset = std::clamp(yawOffset, -MAX_YAW_OFFSET, MAX_YAW_OFFSET);
			m_Rotation.y = m_ChargeStartYaw + yawOffset;

			float desiredPitch = m_Pitch - mouseY * m_MouseSensitivity;
			float pitchOffset = desiredPitch - m_ChargeStartPitch;
			pitchOffset = std::clamp(pitchOffset, -MAX_PITCH_OFFSET, MAX_PITCH_OFFSET);
			m_Pitch = m_ChargeStartPitch + pitchOffset;

			const float maxPitch = DirectX::XM_PIDIV2 - 0.1f;
			m_Pitch = std::clamp(m_Pitch, -maxPitch, maxPitch);
		}
		else // 通常時は自由な視点回転
		{
			m_Rotation.y += mouseX * m_MouseSensitivity;
			m_Pitch -= mouseY * m_MouseSensitivity;

			const float maxPitch = DirectX::XM_PIDIV2 - 0.1f;
			m_Pitch = std::clamp(m_Pitch, -maxPitch, maxPitch);
		}
	}

	// プレイヤーの前方向ベクトルを計算！Y軸回転のみ使用、移動用
	Matrix rotM = Matrix::CreateRotationY(m_Rotation.y);
	m_Forward = Vector3::Transform(Vector3::UnitZ, rotM);
	m_Forward.Normalize();

	// 地面移動に限る
	Vector3 forwardFlat = m_Forward;
	forwardFlat.y = 0.0f;
	if (forwardFlat.LengthSquared() < 1e-6f) forwardFlat = Vector3::UnitZ;
	forwardFlat.Normalize();

	const Vector3 up(0.0f, 1.0f, 0.0f);
	const Vector3 right = up.Cross(forwardFlat);

	// ダッシュ判定
	bool wantsMove = Input::GetKeyPress('W') || Input::GetKeyPress('S') ||
	                 Input::GetKeyPress('A') || Input::GetKeyPress('D');
	bool wantsDash = Input::GetKeyPress(VK_SHIFT) && wantsMove && !m_IsChargingBreak;
	bool shiftHeld = Input::GetKeyPress(VK_SHIFT);

	// Shift を離したらダッシュロックを解除
	if (!shiftHeld)
	{
		m_DashLocked = false;
	}

	// 枯渇ペナルティのタイマー処理
	if (m_IsExhausted)
	{
		m_ExhaustedTimer -= deltaTime;
		if (m_ExhaustedTimer <= 0.0f)
		{
			m_IsExhausted    = false;
			m_ExhaustedTimer = 0.0f;
		}
	}

	if (wantsDash && !m_IsExhausted && !m_DashLocked && m_Stamina > 0.0f)
	{
		m_IsDashing = true;
		m_Stamina -= STAMINA_DRAIN_RATE * deltaTime;
		if (m_Stamina <= 0.0f)
		{
			m_Stamina        = 0.0f;
			m_IsExhausted    = true;
			m_ExhaustedTimer = EXHAUSTED_COOLDOWN;
			m_DashLocked     = true;
			m_IsDashing      = false;
		}
	}
	else
	{
		m_IsDashing = false;
	}

	float currentSpeed = m_MoveSpeed * (m_IsDashing ? DASH_SPEED_MULT : 1.0f);

	// ダッシュエフェクト制御
	if (m_IsDashing && !m_DashEffect)
	{
		m_DashEffect = EffectManager::GetInstance().StartEffect<DashEffect>();
		// EffectManager が削除する直前にコールバックでポインタを無効化
		m_DashEffect->SetOnComplete([this]() { m_DashEffect = nullptr; });
	}
	if (m_DashEffect)
	{
		m_DashEffect->SetActive(m_IsDashing);
	}

	// キーボード入力による移動チャージ中は移動不可
	Vector3 movement = Vector3::Zero;
	if (!m_IsChargingBreak)
	{
		if (Input::GetKeyPress('W')) movement += forwardFlat * currentSpeed;
		if (Input::GetKeyPress('S')) movement -= forwardFlat * currentSpeed;
		if (Input::GetKeyPress('A')) movement -= right * currentSpeed;
		if (Input::GetKeyPress('D')) movement += right * currentSpeed;
	}

	// チートモード時の飛行操作
	if (DebugManager::GetInstance().IsNoClipMode())
	{
		if (!m_IsChargingBreak)
		{
			if (Input::GetKeyPress('Q')) movement.y += m_MoveSpeed;
			if (Input::GetKeyPress('E')) movement.y -= m_MoveSpeed;
		}
	}

	// チートモード時の当たり判定スキップ
	if (DebugManager::GetInstance().IsNoClipMode())
	{
		// 飛行モード時は当たり判定なしで移動
		m_Position += movement;
		collider.center = m_Position;
	}
	else
	{
		// リリースモードでは当たり判定付き移動
		auto weakBlocks = SceneManager::GetInstance().FindAllObjects<Block>();

		auto collidesAt = [&](const DirectX::SimpleMath::Vector3& testPos) -> bool {
			// 壁境界上での偽陽性（接触=衝突扱い）を防ぐため XZ にスキン幅を引く
			static constexpr float SKIN = 0.02f;
			SimpleBoxCollider testCol = collider;
			testCol.size.x = collider.size.x - SKIN * 2.0f;
			testCol.size.z = collider.size.z - SKIN * 2.0f;
			testCol.center = testPos;

			for (auto& wb : weakBlocks)
			{
				if (auto b = wb.lock())
				{
					if (b->IsPendingDestroy())
					{
						continue;
					}

					if (testCol.CheckCollision(b->GetCollider()))
					{
						return true;
					}
				}
			}
			return false;
			};

		DirectX::SimpleMath::Vector3 desired = m_Position;

		if (movement.x != 0.0f) {
			auto test = desired; test.x += movement.x;
			if (!collidesAt(test)) desired.x = test.x;
		}
		if (movement.z != 0.0f) {
			auto test = desired; test.z += movement.z;
			if (!collidesAt(test)) desired.z = test.z;
		}

		m_Position = desired;
		collider.center = m_Position;
	}

	if (IsGoal) return;

	// スタミナ自然回復
	if (!m_IsDashing && m_Stamina < m_MaxStamina && m_Map)
	{
		// プレイヤーのワールド座標からグリッド座標を算出
		const float HALF_BLOCK = MAP::Config::BLOCK_SIZE / 2.0f;

		// マップサイズはランタイムで難易度によって変わるため、MakeMap から取得する
		const float MAP_CX = m_Map->GetSizeX() / 2.0f * MAP::Config::BLOCK_SIZE;
		const float MAP_CY = m_Map->GetSizeY() / 2.0f * MAP::Config::BLOCK_SIZE;

		int gx = static_cast<int>(std::round((HALF_BLOCK + MAP_CX - m_Position.x) / MAP::Config::BLOCK_SIZE));
		int gy = static_cast<int>(std::round((HALF_BLOCK - MAP_CY + (-m_Position.z)) / MAP::Config::BLOCK_SIZE));

		// ゴールからの距離を計算
		int sx, sy, goalX, goalY;
		m_Map->GetStartGoal(sx, sy, goalX, goalY);
		int distGoal = std::abs(goalX - gx) + std::abs(goalY - gy);

		float regenRate;
		if (distGoal <= m_Map->GetGoalNearRadius())   // 赤ゾーン（ゴール近接）
			regenRate = STAMINA_REGEN_NEAR;
		else if (distGoal <= m_Map->GetGoalMidRadius()) // 青ゾーン（中距離）
			regenRate = STAMINA_REGEN_MID;    // 青ゾーン中間
		else
			regenRate = STAMINA_REGEN_FAR;    // 緑ゾーン遠距離

		// 枯渇ペナルティ中は回復しない
		if (!m_IsExhausted)
		{
			m_Stamina = std::min(m_Stamina + regenRate * deltaTime, m_MaxStamina);
		}
	}

	// ゴール判定（デバッグモード・リリースモード・通用）
	auto weakPole = SceneManager::GetInstance().FindObject<Pole>();

	if (auto p = weakPole.lock())
	{
		if (collider.CheckCollision(p->GetCollider()))
		{
			
			if (m_HasKey)
			{
				IsGoal = true;
				m_CanMove = false;// 移動不可にする
				SceneManager::GetInstance().ChangeScene<GameClearScene>(std::make_unique<FadeTransition>(3.0f));
			}
			else
			{
				//カギを持っていない場合の処理
				// ここで「カギが必要です！」といった画像を表示するフラグを立てる等の処理を行う
				std::cout << "You need the Key to goal!" << std::endl;

			}
		}
	}
}


//=======================================
// 透明化開始処理
//=======================================
void Player::StartInvisible(float duration)
{
	m_IsInvisible = true;
	m_InvisibleTimer = duration;
	std::cout << "!!! POWER UP START (Time: " << duration << ") !!!\n";
}


//=======================================
// 透明化ストック追加処理
//=======================================
void Player::AddInvisibleStock(int amount)
{
	m_InvisibleStock += amount;

	// 最大値を超えないようにキャップする
	if (m_InvisibleStock > m_MaxInvisibleStock)
	{
		m_InvisibleStock = m_MaxInvisibleStock;
	}
}



//=======================================
// 描画処理
//=======================================
void Player::Draw()
{

	//// SRT行列作成
	//Matrix r = Matrix::CreateRotationY(m_Rotation.y + DirectX::XM_PI); // Y軸回転のみ
	//Matrix t = Matrix::CreateTranslation(m_Position.x, m_Position.y, m_Position.z);
	//Matrix s = Matrix::CreateScale(m_Scale.x, m_Scale.y, m_Scale.z);

	//Matrix worldmtx;
	//worldmtx = s * r * t;
	//Renderer::SetWorldMatrix(&worldmtx); // GPUにセット

	//m_Shader.SetGPU();

	//// インデックスバッファ・頂点バッファをセット
	//m_MeshRenderer.BeforeDraw();

	////マテリアル数分ループ
	//for (int i = 0; i < m_subsets.size(); i++)
	//{
	//	// マテリアルをセット(サブセット内にあるマテリアルインデックスを使用)
	//	m_Materiales[m_subsets[i].MaterialIdx]->SetGPU();


	//	if (m_Materiales[m_subsets[i].MaterialIdx]->isTextureEnable())
	//	{
	//		m_Textures[m_subsets[i].MaterialIdx]->SetGPU();
	//	}

	//	m_MeshRenderer.DrawSubset(
	//		m_subsets[i].IndexNum, // 描画するインデックス数
	//		m_subsets[i].IndexBase, // 最初のインデックスバッファの位置
	//		m_subsets[i].VertexBase); // 頂点バッファの最初から使用
	//}
}

//=======================================
// 終了処理
//=======================================
void Player::Uninit() {}




//=======================================
// プレイヤー前方のブロック取得
//=======================================
std::weak_ptr<Block> Player::GetBlockInFront() const
{
	if (!m_Map) return std::weak_ptr<Block>();

	// プレイヤーの前方向ベクトルを取得
	Vector3 forwardFlat = m_Forward;
	forwardFlat.y = 0.0f;

	if (forwardFlat.LengthSquared() < 1e-6f)
	{
		forwardFlat = Vector3::UnitZ;
	}
	forwardFlat.Normalize();

	// すべてのブロックを取得
	auto blocks = SceneManager::GetInstance().FindAllObjects<Block>();

	std::weak_ptr<Block> closestBlock;
	float closestDist = WALL_BREAK_RANGE;

	for (auto& weakBlock : blocks)
	{
		if (auto block = weakBlock.lock())
		{
			// 既に破壊フラグが立っているブロックはスキップ
			if (block->IsPendingDestroy())
			{
				continue;
			}

			// ブロックとの距離を計算
			Vector3 blockPos = block->GetPosition();
			Vector3 toBlock = blockPos - m_Position;
			toBlock.y = 0.0f; // 高さは無視

			float dist = toBlock.Length();

			if (dist > WALL_BREAK_RANGE)// 範囲外のブロックはスキップ
			{
				continue;
			}

			// プレイヤーの前方方向にあるかチェック
			toBlock.Normalize();
			float dot = forwardFlat.Dot(toBlock);

			// 前方にあり、最も近いブロックを選択
			if (dot > 0.7f && dist < closestDist)
			{
				closestDist = dist;
				closestBlock = weakBlock;
			}
		}
	}

	return closestBlock;
}



//=======================================
// 壁破壊処理
//=======================================
void Player::TryBreakWall()
{

	if (!m_Map)
	{
		std::cout << "Error: Map reference is null\n";
		return;
	}

	// チャージ開始時に記録したターゲットブロックを使用
	auto block = m_TargetBlock.lock();

	if (!block)
	{
		return;
	}

	// ブロックが既に破壊予定の場合はスキップ
	if (block->IsPendingDestroy())
	{
		return;
	}

	// ブロックのグリッド座標を取得
	int gridX = block->GetGridX();
	int gridY = block->GetGridY();

	if (gridX < 0 || gridY < 0)
	{
		std::cout << "Error: Invalid grid coordinates\n";
		return;
	}

	// マップグリッドを更新(壁を通路に変更)
	m_Map->DestroyWall(gridX, gridY);

	// ブロックオブジェクトを削除
	block->Destroy();

	std::cout << "Wall broken at grid (" << gridX << ", " << gridY << ")\n";
}


