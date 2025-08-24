#include "Player.h"
#include <memory>
#include "StaticMesh.h"
#include "utility.h"
#include "Game.h"
#include "Collision.h"
#include"Block.h"

#include <type_traits>

using namespace std;
using namespace DirectX::SimpleMath;

//コンストラクタ
Player::Player(const Vector3& pos, const Vector3& size)
	: ColliderObject(pos, size),
	m_MouseSensitivity(0.003f),
	m_MoveSpeed(6.5f),
	m_MouseCaptured(true),
	m_Forward(Vector3::UnitZ),
	m_Pitch(0.0f)
{

}

// デストラクタ
Player::~Player() {}

//=======================================
// 初期化処理
//=======================================
void Player::Init()
{
	// メッシュ読み込み
	StaticMesh staticmesh;

	// 3Dモデルデータ
	std::u8string modelFile = u8"assets/model/player/Player.fbx";

	// テクスチャディレクトリ
	std::string texDirectory = "assets/texture/terain.png";

	// Meshを読み込む
	std::string tmpStr1(reinterpret_cast<const char*>(modelFile.c_str()), modelFile.size());
	staticmesh.Load(tmpStr1, texDirectory);

	m_MeshRenderer.Init(staticmesh);

	// シェーダオブジェクト生成
	m_Shader.Create("shader/litTextureVS.hlsl", "shader/litTexturePS.hlsl");

	// サブセット情報取得
	m_subsets = staticmesh.GetSubsets();

	// テクスチャ情報取得
	m_Textures = staticmesh.GetTextures();

	// マテリアル情報取得
	vector<MATERIAL> materials = staticmesh.GetMaterials();

	// マテリアル数分ループ
	for (int i = 0; i < materials.size(); ++i)
	{
		// マテリアルオブジェクト生成
		std::unique_ptr<Material> m = std::make_unique<Material>();

		// マテリアル情報をセット
		m->Create(materials[i]);

		// マテリアルオブジェクトを配列に追加
		m_Materiales.push_back(std::move(m));
	}
}


//========================================
//当たり判定チェック関数
//========================================
bool Player::CheckCollisionWithBlocks(const Vector3& newPosition)
{

#ifdef _DEBUG
	return false;// デバッグビルド時は当たり判定をスキップ
#endif

	// 新しい位置でのコライダーを作成
	SimpleBoxCollider testCollider(newPosition, collider.size);

	// ゲーム内のすべてのBlockオブジェクトをチェック
	auto blocks = Game::GetInstance().FindAllObjects<Block>();
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
void Player::Update()
{
	collider.center = m_Position; // コライダーの中心位置を更新

	// マウスキャプチャ状態の時のみマウス操作を処理
	if (m_MouseCaptured)
	{
		int mouseX, mouseY;
		Input::GetMouseMove(&mouseX, &mouseY);

		// プレイヤーの回転を更新
		m_Rotation.y += mouseX * m_MouseSensitivity;   // yaw（左右回転）
		m_Pitch -= mouseY * m_MouseSensitivity;        // pitch（上下視点）

		// ピッチ角度を制限（真上・真下を向きすぎないよう制限）
		const float maxPitch = DirectX::XM_PIDIV2 - 0.1f;
		m_Pitch = std::clamp(m_Pitch, -maxPitch, maxPitch);
	}

	// プレイヤーの前方向ベクトルを計算（Y軸回転のみ使用、移動用）
	Matrix rotM = Matrix::CreateRotationY(m_Rotation.y);
	m_Forward = Vector3::Transform(Vector3::UnitZ, rotM);
	m_Forward.Normalize();

	// 地面移動に限定（上下へは動かない）
	Vector3 forwardFlat = m_Forward;
	forwardFlat.y = 0.0f;
	if (forwardFlat.LengthSquared() < 1e-6f) forwardFlat = Vector3::UnitZ;
	forwardFlat.Normalize();

	const Vector3 up(0.0f, 1.0f, 0.0f);
	const Vector3 right = up.Cross(forwardFlat);

	// キーボード入力による移動
	Vector3 movement = Vector3::Zero;
	if (Input::GetKeyPress('W')) movement += forwardFlat * m_MoveSpeed;
	if (Input::GetKeyPress('S')) movement -= forwardFlat * m_MoveSpeed;
	if (Input::GetKeyPress('A')) movement -= right * m_MoveSpeed;
	if (Input::GetKeyPress('D')) movement += right * m_MoveSpeed;
#ifdef _DEBUG
	if (Input::GetKeyPress('Q')) movement.y += m_MoveSpeed;
	if (Input::GetKeyPress('E')) movement.y -= m_MoveSpeed;

	m_Position += movement;
	collider.center = m_Position;
	return;
#endif

	// -------- Collision detection added ---------
	// 各軸ごとに仮位置を試し、ブロックと衝突する場合はその軸の移動を取り消す
	auto weakBlocks = Game::GetInstance().FindAllObjects<Block>();

	// 指定位置に仮置きした場合にブロックと衝突するか？
	auto collidesAt = [&](const DirectX::SimpleMath::Vector3& testPos) -> bool {
		SimpleBoxCollider testCol = collider; // 既存と同サイズ
		testCol.center = testPos;

		for (auto& wb : weakBlocks) {
			if (auto b = wb.lock()) 
			{
				if (testCol.CheckCollision(b->GetCollider())) {
					return true; // 1つでも当たれば衝突
				}
			}
		}
		return false;
		};

	// 軸ごとに仮移動 → 衝突した軸の移動だけ潰す
	DirectX::SimpleMath::Vector3 desired = m_Position;

	if (movement.x != 0.0f) {
		auto test = desired; test.x += movement.x;
		if (!collidesAt(test)) desired.x = test.x; // 当たらなければ反映
	}
	if (movement.z != 0.0f) {
		auto test = desired; test.z += movement.z;
		if (!collidesAt(test)) desired.z = test.z;
	}

	// 反映
	m_Position = desired;
	collider.center = m_Position;
}


//=======================================
// 描画処理
//=======================================
void Player::Draw()
{

	// SRT情報作成
	Matrix r = Matrix::CreateRotationY(m_Rotation.y + DirectX::XM_PI); // Y軸回転のみ
	Matrix t = Matrix::CreateTranslation(m_Position.x, m_Position.y, m_Position.z);
	Matrix s = Matrix::CreateScale(m_Scale.x, m_Scale.y, m_Scale.z);

	Matrix worldmtx;
	worldmtx = s * r * t;
	Renderer::SetWorldMatrix(&worldmtx); // GPUにセット

	m_Shader.SetGPU();

	// インデックスバッファ・頂点バッファをセット
	m_MeshRenderer.BeforeDraw();

	//マテリアル数分ループ 
	for (int i = 0; i < m_subsets.size(); i++)
	{
		// マテリアルをセット(サブセット情報の中にあるマテリアルインデックスを使用)
		m_Materiales[m_subsets[i].MaterialIdx]->SetGPU();


		if (m_Materiales[m_subsets[i].MaterialIdx]->isTextureEnable())
		{
			m_Textures[m_subsets[i].MaterialIdx]->SetGPU();
		}

		m_MeshRenderer.DrawSubset(
			m_subsets[i].IndexNum, // 描画するインデックス数
			m_subsets[i].IndexBase, // 最初のインデックスバッファの位置	
			m_subsets[i].VertexBase); // 頂点バッファの最初から使用
	}
}

//=======================================
// 終了処理
//=======================================
void Player::Uninit() {}


