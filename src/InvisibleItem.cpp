#include "pch.h"
#include "InvisibleItem.h"
#include "StaticMesh.h"
#include "utility.h"
#include "SceneManager.h"
#include "Collision.h"
#include "Player.h"
#include"Renderer.h"
#include "EffectManager.h"
#include "InvisibleEffect.h"

using namespace std;
using namespace DirectX::SimpleMath;

//コンストラクタ
InvisibleItem::InvisibleItem(
	const DirectX::SimpleMath::Vector3& pos,
	const DirectX::SimpleMath::Vector3& size)
	:ColliderObject(pos, size)
{
}

// チE��トラクタ
InvisibleItem::~InvisibleItem() {}

//=======================================
// 初期化�E琁E
//=======================================
void InvisibleItem::Init()
{
	// メチE��ュ読み込み
	StaticMesh staticmesh;

	// 3DモチE��チE�Eタ
	std::u8string modelFile = u8"assets/model/InvisibleItem/InvisibleItem.fbx";

	// チE��スチャチE��レクトリ
	std::string texDirectory = "assets/model/InvisibleItem";

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

//=======================================
// 更新処琁E
//=======================================
void InvisibleItem::Update(float deltaTime)
{
	m_Rotation.y += 0.01f;//アイチE��の回転

	//プレイヤーを取征E
	auto playerWeak = SceneManager::GetInstance().FindObject<Player>();
	if (auto player = playerWeak.lock())
	{
		//プレイヤーと自刁E�Eコライダーで衝突判宁E
		if (this->collider.CheckCollision(player->GetCollider()))
		{
			if (player->GetInvisibleStock() < player->GetMaxInvisibleStock())
			{
				player->AddInvisibleStock(1); // ストックめE増やぁE
				this->Destroy();              // 自刁E��破棁E
			}
		}
	}
}

//=======================================
// 描画処琁E
//=======================================
void InvisibleItem::Draw()
{
		// SRT惁E��作�E
		Matrix r = Matrix::CreateFromYawPitchRoll(m_Rotation.y, m_Rotation.x, m_Rotation.z);
		Matrix t = Matrix::CreateTranslation(m_Position.x, m_Position.y, m_Position.z);
		Matrix s = Matrix::CreateScale(m_Scale.x, m_Scale.y, m_Scale.z);

		Matrix worldmtx;
		worldmtx = s * r * t;
		Renderer::SetWorldMatrix(&worldmtx); // GPUにセチE��

		m_Shader.SetGPU();

		// インチE��クスバッファ・頂点バッファをセチE��
		m_MeshRenderer.BeforeDraw();

		//マテリアル数刁E��ーチE
		for (int i = 0; i < m_subsets.size(); i++)
		{
			// マテリアルをセチE��(サブセチE��惁E��の中にあるマテリアルインチE��クスを使用)
			m_Materiales[m_subsets[i].MaterialIdx]->SetGPU();


			if (m_Materiales[m_subsets[i].MaterialIdx]->isTextureEnable())
			{
				m_Textures[m_subsets[i].MaterialIdx]->SetGPU();
			}

			m_MeshRenderer.DrawSubset(
				m_subsets[i].IndexNum, // 描画するインチE��クス数
				m_subsets[i].IndexBase, // 最初�EインチE��クスバッファの位置	
				m_subsets[i].VertexBase); // 頂点バッファの最初から使用
		}
	

	//#ifdef _DEBUG
	//	Matrix colliderWorldMatrix = r * t;
	//	// スケールを含めなぁE���Eを渡して描画します、E
	//	collider.DrawDebugCollider(Game::GetInstance().GetMainCamera(), colliderWorldMatrix);
	//#endif
}



//=======================================
// 終亁E�E琁E
//=======================================
void InvisibleItem::Uninit()
{
}
