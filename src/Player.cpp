#include "pch.h"
#include "Player.h"
#include "StaticMesh.h"
#include "utility.h"
#include "Game.h"
#include "Collision.h"
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

// チE��トラクタ
Player::~Player() {}


//=======================================
// 初期化�E琁E
//=======================================
void Player::Init()
{
	// メチE��ュ読み込み
	StaticMesh staticmesh;

	// 3DモチE��チE�Eタ
	std::u8string modelFile = u8"assets/model/player/Player.fbx";

	// チE��スチャチE��レクトリ
	std::string texDirectory = "assets/texture/terain.png";

	// Meshを読み込む
	std::string tmpStr1(reinterpret_cast<const char*>(modelFile.c_str()), modelFile.size());
	staticmesh.Load(tmpStr1, texDirectory);

	m_MeshRenderer.Init(staticmesh);

	//当たり判定用のサイズを設宁E
	collider.size = GetScale() * (staticmesh.GetMax() - staticmesh.GetMin());

	// シェーダオブジェクト生戁E
	m_Shader.Create("shader/litTextureVS.hlsl", "shader/litTexturePS.hlsl");

	// サブセチE��惁E��取征E
	m_subsets = staticmesh.GetSubsets();

	// チE��スチャ惁E��取征E
	m_Textures = staticmesh.GetTextures();

	// マテリアル惁E��取征E
	vector<MATERIAL> materials = staticmesh.GetMaterials();

	// マテリアル数刁E��ーチE
	for (int i = 0; i < materials.size(); ++i)
	{
		// マテリアルオブジェクト生戁E
		std::unique_ptr<Material> m = std::make_unique<Material>();

		// マテリアル惁E��をセチE��
		m->Create(materials[i]);

		// マテリアルオブジェクトを配�Eに追加
		m_Materiales.push_back(std::move(m));
	}
}


//========================================
//当たり判定チェチE��関数
//========================================
bool Player::CheckCollisionWithBlocks(const Vector3& newPosition)
{

	if (DebugManager::GetInstance().IsNoClipMode())
	{
		return false;// ノ�EクリチE�Eモードなら衝突しなぁE
	}

	// 新しい位置でのコライダーを作�E
	SimpleBoxCollider testCollider(newPosition, collider.size);

	// ゲーム冁E�EすべてのBlockオブジェクトをチェチE��
	auto blocks = SceneManager::GetInstance().FindAllObjects<Block>();
	for (auto& weakBlock : blocks)
	{
		// weak_ptrをshared_ptrに変換してチェチE��
		if (auto block = weakBlock.lock())
		{
			// 当たり判定をチェチE��
			if (testCollider.CheckCollision(block->GetCollider()))
			{
				return true; // 衝突してぁE��
			}
		}
	}
	return false; // 衝突してぁE��ぁE
}




//=======================================
// 更新処琁E
//=======================================
void Player::Update(float deltaTime)
{
	collider.center = m_Position; // コライダーの中忁E��置を更新

	// Fキーで透�E化発勁E
	if (Input::GetKeyTrigger('F'))
	{
		if (!m_IsInvisible && m_InvisibleStock > 0)
		{
			m_InvisibleStock--; // 減らすだけ。表示は関知しなぁE��E
			StartInvisible(10.0f);

			EffectManager::GetInstance().StartEffect<InvisibleEffect>(m_InvisibleTime);//透�E化エフェクト開姁E
		}
	}


	//透�E化タイマ�E処琁E
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

	// 移動可能フラグが立ってぁE��ぁE��合�E処琁E��スキチE�E
	if (!m_CanMove) return;


	// 壁破壊チャージ処琁E
	bool currentButtonState = Input::GetKeyPress(VK_LBUTTON);

	if (currentButtonState)
	{
		if (m_WallBreakJustCompleted)
		{
			// 壁破壊直後�E押しっぱなしでは再チャージしなぁE
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
			// チャージ継綁E
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

	// マウスキャプチャ状態�E時�Eみマウス操作を処琁E
	if (m_MouseCaptured)
	{
		int mouseX, mouseY;
		Input::GetMouseMove(&mouseX, &mouseY);

		if (m_IsChargingBreak) // 破壊チャージ中は視点回転を制陁E
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
		else // 通常時�E自由な視点回転
		{
			m_Rotation.y += mouseX * m_MouseSensitivity;
			m_Pitch -= mouseY * m_MouseSensitivity;

			const float maxPitch = DirectX::XM_PIDIV2 - 0.1f;
			m_Pitch = std::clamp(m_Pitch, -maxPitch, maxPitch);
		}
	}

	// プレイヤーの前方向�Eクトルを計算！E軸回転のみ使用、移動用�E�E
	Matrix rotM = Matrix::CreateRotationY(m_Rotation.y);
	m_Forward = Vector3::Transform(Vector3::UnitZ, rotM);
	m_Forward.Normalize();

	// 地面移動に限宁E
	Vector3 forwardFlat = m_Forward;
	forwardFlat.y = 0.0f;
	if (forwardFlat.LengthSquared() < 1e-6f) forwardFlat = Vector3::UnitZ;
	forwardFlat.Normalize();

	const Vector3 up(0.0f, 1.0f, 0.0f);
	const Vector3 right = up.Cross(forwardFlat);

	// ダチE��ュ判宁E
	bool wantsMove = Input::GetKeyPress('W') || Input::GetKeyPress('S') ||
	                 Input::GetKeyPress('A') || Input::GetKeyPress('D');
	bool wantsDash = Input::GetKeyPress(VK_SHIFT) && wantsMove && !m_IsChargingBreak;

	if (wantsDash && m_Stamina > STAMINA_DASH_THRESHOLD)
	{
		m_IsDashing = true;
		m_Stamina -= STAMINA_DRAIN_RATE * deltaTime;
		if (m_Stamina < 0.0f) m_Stamina = 0.0f;
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

	// キーボ�Eド�E力による移勁Eチャージ中は移動不可)
	Vector3 movement = Vector3::Zero;
	if (!m_IsChargingBreak)
	{
		if (Input::GetKeyPress('W')) movement += forwardFlat * currentSpeed;
		if (Input::GetKeyPress('S')) movement -= forwardFlat * currentSpeed;
		if (Input::GetKeyPress('A')) movement -= right * currentSpeed;
		if (Input::GetKeyPress('D')) movement += right * currentSpeed;
	}

	// チE��チE��モード�E飛行操佁E
	if (DebugManager::GetInstance().IsNoClipMode())
	{
		if (!m_IsChargingBreak)
		{
			if (Input::GetKeyPress('Q')) movement.y += m_MoveSpeed;
			if (Input::GetKeyPress('E')) movement.y -= m_MoveSpeed;
		}
	}

	// チE��チE��モード�E当たり判定スキチE�E
	if (DebugManager::GetInstance().IsNoClipMode())
	{
		// 飛行モード時は当たり判定なしで移勁E
		m_Position += movement;
		collider.center = m_Position;
	}
	else
	{
		// リリースモードでは当たり判定付き移動�E琁E
		auto weakBlocks = SceneManager::GetInstance().FindAllObjects<Block>();

		auto collidesAt = [&](const DirectX::SimpleMath::Vector3& testPos) -> bool {
			SimpleBoxCollider testCol = collider;
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

	// スタミナ自然回復�E�ダチE��ュしてぁE��ぁE���E�E
	if (!m_IsDashing && m_Stamina < m_MaxStamina && m_Map)
	{
		// プレイヤーのワールド座標からグリチE��座標を送E��E
		const float HALF_BLOCK = MAP::Config::BLOCK_SIZE / 2.0f;
		const float MAP_CX = MAP::Config::MaxX / 2.0f * MAP::Config::BLOCK_SIZE;
		const float MAP_CY = MAP::Config::MaxY / 2.0f * MAP::Config::BLOCK_SIZE;

		int gx = static_cast<int>(std::round((HALF_BLOCK + MAP_CX - m_Position.x) / MAP::Config::BLOCK_SIZE));
		int gy = static_cast<int>(std::round((HALF_BLOCK - MAP_CY + (-m_Position.z)) / MAP::Config::BLOCK_SIZE));

		// ゴールからの距離を計箁E
		int sx, sy, goalX, goalY;
		m_Map->GetStartGoal(sx, sy, goalX, goalY);
		int distGoal = std::abs(goalX - gx) + std::abs(goalY - gy);

		float regenRate;
		if (distGoal <= MAP::Config::GoalNearRadius)
			regenRate = STAMINA_REGEN_NEAR;   // 赤ゾーン�E�遅ぁE��E
		else if (distGoal <= MAP::Config::GoalMidRadius)
			regenRate = STAMINA_REGEN_MID;    // 青ゾーン�E�中間！E
		else
			regenRate = STAMINA_REGEN_FAR;    // 緑ゾーン�E�速い�E�E

		m_Stamina = std::min(m_Stamina + regenRate * deltaTime, m_MaxStamina);
	}

	// ゴール判定（デバッグモード�Eリリースモード�E通！E
	auto weakPole = SceneManager::GetInstance().FindObject<Pole>();

	if (auto p = weakPole.lock())
	{
		if (collider.CheckCollision(p->GetCollider()))
		{
			//カギを持ってぁE��かチェチE��
			if (m_HasKey)
			{
				IsGoal = true;
				m_CanMove = false;// 移動不可にする
				SceneManager::GetInstance().ChangeScene<GameClearScene>(std::make_unique<FadeTransition>(3.0f));
			}
			else
			{
				//カギを持ってぁE��ぁE��合�E処琁E
				// ここで「カギが忁E��です！」とぁE��画像を表示するフラグを立てる等�E処琁E��行う
				std::cout << "You need the Key to goal!" << std::endl;

			}
		}
	}
}


//=======================================
// 透�E化開始�E琁E
//=======================================
void Player::StartInvisible(float duration)
{
	m_IsInvisible = true;
	m_InvisibleTimer = duration;
	std::cout << "!!! POWER UP START (Time: " << duration << ") !!!\n";
}


//=======================================
// 透�E化ストック追加処琁E
//=======================================
void Player::AddInvisibleStock(int amount)
{
	m_InvisibleStock += amount;

	// 最大値を趁E��なぁE��ぁE��キャチE�Eする
	if (m_InvisibleStock > m_MaxInvisibleStock)
	{
		m_InvisibleStock = m_MaxInvisibleStock;
	}
}



//=======================================
// 描画処琁E
//=======================================
void Player::Draw()
{

	//// SRT惁E��作�E
	//Matrix r = Matrix::CreateRotationY(m_Rotation.y + DirectX::XM_PI); // Y軸回転のみ
	//Matrix t = Matrix::CreateTranslation(m_Position.x, m_Position.y, m_Position.z);
	//Matrix s = Matrix::CreateScale(m_Scale.x, m_Scale.y, m_Scale.z);

	//Matrix worldmtx;
	//worldmtx = s * r * t;
	//Renderer::SetWorldMatrix(&worldmtx); // GPUにセチE��

	//m_Shader.SetGPU();

	//// インチE��クスバッファ・頂点バッファをセチE��
	//m_MeshRenderer.BeforeDraw();

	////マテリアル数刁E��ーチE
	//for (int i = 0; i < m_subsets.size(); i++)
	//{
	//	// マテリアルをセチE��(サブセチE��惁E��の中にあるマテリアルインチE��クスを使用)
	//	m_Materiales[m_subsets[i].MaterialIdx]->SetGPU();


	//	if (m_Materiales[m_subsets[i].MaterialIdx]->isTextureEnable())
	//	{
	//		m_Textures[m_subsets[i].MaterialIdx]->SetGPU();
	//	}

	//	m_MeshRenderer.DrawSubset(
	//		m_subsets[i].IndexNum, // 描画するインチE��クス数
	//		m_subsets[i].IndexBase, // 最初�EインチE��クスバッファの位置	
	//		m_subsets[i].VertexBase); // 頂点バッファの最初から使用
	//}
}

//=======================================
// 終亁E�E琁E
//=======================================
void Player::Uninit() {}




//=======================================
// プレイヤー前方のブロチE��取征E
//=======================================
std::weak_ptr<Block> Player::GetBlockInFront() const
{
	if (!m_Map) return std::weak_ptr<Block>();

	// プレイヤーの前方向�Eクトルを取征E
	Vector3 forwardFlat = m_Forward;
	forwardFlat.y = 0.0f;

	if (forwardFlat.LengthSquared() < 1e-6f)
	{
		forwardFlat = Vector3::UnitZ;
	}
	forwardFlat.Normalize();

	// すべてのブロチE��を取征E
	auto blocks = SceneManager::GetInstance().FindAllObjects<Block>();

	std::weak_ptr<Block> closestBlock;
	float closestDist = WALL_BREAK_RANGE;

	for (auto& weakBlock : blocks)
	{
		if (auto block = weakBlock.lock())
		{
			// 既に破棁E��ラグが立ってぁE��ブロチE��はスキチE�E
			if (block->IsPendingDestroy())
			{
				continue;
			}

			// ブロチE��との距離を計箁E
			Vector3 blockPos = block->GetPosition();
			Vector3 toBlock = blockPos - m_Position;
			toBlock.y = 0.0f; // 高さは無要E

			float dist = toBlock.Length();

			if (dist > WALL_BREAK_RANGE)// 篁E��夁E
			{
				continue;
			}

			// プレイヤーの前方方向にあるかチェチE��
			toBlock.Normalize();
			float dot = forwardFlat.Dot(toBlock);

			// 前方にあり篁E��冁E��、最も近いブロチE��を選抁E
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
// 壁破壊�E琁E
//=======================================
void Player::TryBreakWall()
{

	if (!m_Map)
	{
		std::cout << "Error: Map reference is null\n";
		return;
	}

	// チャージ開始時に記録したターゲチE��ブロチE��を使用
	auto block = m_TargetBlock.lock();

	if (!block)
	{
		return;
	}

	// ブロチE��が既に破棁E��定�E場合�EスキチE�E
	if (block->IsPendingDestroy())
	{
		return;
	}

	// ブロチE��のグリチE��座標を取征E
	int gridX = block->GetGridX();
	int gridY = block->GetGridY();

	if (gridX < 0 || gridY < 0)
	{
		std::cout << "Error: Invalid grid coordinates\n";
		return;
	}

	// マップグリチE��を更新(壁�E通路に変更)
	m_Map->DestroyWall(gridX, gridY);

	// ブロチE��オブジェクトを削除
	block->Destroy();

	std::cout << "Wall broken at grid (" << gridX << ", " << gridY << ")\n";
}