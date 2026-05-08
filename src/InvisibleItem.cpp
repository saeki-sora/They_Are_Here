#include "pch.h"
#include "InvisibleItem.h"
#include "StaticMesh.h"
#include "utility.h"
#include "SceneManager.h"
#include "Player.h"
#include"Renderer.h"
#include "EffectManager.h"
#include "InvisibleEffect.h"
#include "DebugManager.h"
#include "Game.h"

using namespace std;
using namespace DirectX::SimpleMath;

//コンストラクタ
InvisibleItem::InvisibleItem(
	const DirectX::SimpleMath::Vector3& pos,
	const DirectX::SimpleMath::Vector3& size)
	:ColliderObject(pos, size)
{
}

//デストラクタ
InvisibleItem::~InvisibleItem() {}

//=======================================
// 初期化処理
//=======================================
void InvisibleItem::Init()
{
	// メッシュ読み込み
	StaticMesh staticmesh;

	// 3Dモデルファイル
	std::u8string modelFile = u8"assets/model/InvisibleItem/InvisibleItem.fbx";

	// テクスチャディレクトリ
	std::string texDirectory = "assets/model/InvisibleItem";

	// Meshを読み込む
	std::string tmpStr1(reinterpret_cast<const char*>(modelFile.c_str()), modelFile.size());
	staticmesh.Load(tmpStr1, texDirectory);

	m_MeshRenderer.Init(staticmesh);

	//当たり判定用のサイズを設定
	collider.size = GetScale() * (staticmesh.GetMax() - staticmesh.GetMin());

	// シェーダオブジェクト生成
	m_Shader.Create("shader/litTextureVS.hlsl", "shader/litTexturePS.hlsl");

	// サブセット取得
	m_subsets = staticmesh.GetSubsets();

	// テクスチャ取得
	m_Textures = staticmesh.GetTextures();

	// マテリアル取得
	vector<MATERIAL> materials = staticmesh.GetMaterials();

	// マテリアル数分ループ
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

//=======================================
// 更新処理
//=======================================
void InvisibleItem::Update(float deltaTime)
{
	m_Rotation.y += 0.01f;
	collider.rotation = DirectX::SimpleMath::Quaternion::CreateFromYawPitchRoll(m_Rotation.y, m_Rotation.x, m_Rotation.z);

	//プレイヤーを取得
	auto playerWeak = SceneManager::GetInstance().FindObject<Player>();
	if (auto player = playerWeak.lock())
	{
		//プレイヤーと自分のコライダーで衝突判定
		if (this->collider.CheckCollision(player->GetCollider()))
		{
			if (player->GetInvisibleStock() < player->GetMaxInvisibleStock())
			{
				player->AddInvisibleStock(1); // ストックを増やす
				this->Destroy();              // 自分を破棄
			}
		}
	}
}

//=======================================
// 描画処理
//=======================================
void InvisibleItem::Draw()
{
		// SRT行列作成
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
			// マテリアルをセット(サブセットの中にあるマテリアルインデックスを使用)
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
	

    if (DebugManager::GetInstance().ShouldShowColliders()) { collider.DrawDebugCollider(Game::GetInstance().GetMainCamera()); }
}



//=======================================
// 終了処理
//=======================================
void InvisibleItem::Uninit()
{
}

