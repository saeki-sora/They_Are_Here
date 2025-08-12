#include "Player.h"
#include <memory>
#include "StaticMesh.h"
#include "utility.h"
#include "Game.h"
#include "Collision.h"

using namespace std;
using namespace DirectX::SimpleMath;

//コンストラクタ
Player::Player(const Vector3& pos, const Vector3& size)
	: ColliderObject(pos, size),
	m_MouseSensitivity(0.005f),
	m_MoveSpeed(0.5f),
	m_MouseCaptured(true),  // 追加
	m_Forward(Vector3::UnitZ)  // 追加
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
	for (int i = 0; i < materials.size(); i++)
	{
		// マテリアルオブジェクト生成
		std::unique_ptr<Material> m = std::make_unique<Material>();

		// マテリアル情報をセット
		m->Create(materials[i]);

		// マテリアルオブジェクトを配列に追加
		m_Materiales.push_back(std::move(m));
	}
}

//=======================================
// 更新処理
//=======================================
void Player::Update()
{
	collider.center = m_Position;// コライダーの中心位置を更新

	if (m_MouseCaptured)
	{
		int mouseX, mouseY;
		Input::GetMouseMove(&mouseX, &mouseY);

		m_Rotation.y += mouseX * m_MouseSensitivity;   // yaw
		m_Rotation.x -= mouseY * m_MouseSensitivity;   // pitch

		const float maxPitch = DirectX::XM_PIDIV2 - 0.1f;
		m_Rotation.x = std::clamp(m_Rotation.x, -maxPitch, maxPitch);
	}

	const Matrix rotM =
		Matrix::CreateFromYawPitchRoll(m_Rotation.y, m_Rotation.x, m_Rotation.z);

	m_Forward = Vector3::Transform(Vector3::UnitZ, rotM);
	m_Forward.Normalize();

    // 地面移動に限定（上下へは動かない）
	Vector3 forwardFlat = m_Forward;
	forwardFlat.y = 0.0f;
	if (forwardFlat.LengthSquared() < 1e-6f) forwardFlat = Vector3::UnitZ;
	forwardFlat.Normalize();

	const Vector3 up(0.0f, 1.0f, 0.0f);
	const Vector3 right = up.Cross(forwardFlat);  // 版によっては forwardFlat.Cross(up) にする

	// キーボード入力による移動
	Vector3 movement = Vector3::Zero;
	if (Input::GetKeyPress('W')) movement += forwardFlat * m_MoveSpeed;
	if (Input::GetKeyPress('S')) movement -= forwardFlat * m_MoveSpeed;
	if (Input::GetKeyPress('A')) movement -= right * m_MoveSpeed;
	if (Input::GetKeyPress('D')) movement += right * m_MoveSpeed;


#ifdef _DEBUG
	if (Input::GetKeyPress('Q')) movement.y += m_MoveSpeed; // デバッグ用上下
	if (Input::GetKeyPress('E')) movement.y -= m_MoveSpeed;
#endif

	// プレイヤーの位置とコライダーの中心位置を更新
	m_Position += movement;
	collider.center = m_Position;

}

//=======================================
// 描画処理
//=======================================
void Player::Draw()
{

	// SRT情報作成
	Matrix r = Matrix::CreateFromYawPitchRoll(m_Rotation.y, m_Rotation.x, m_Rotation.z);
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


