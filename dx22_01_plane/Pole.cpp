#include "Pole.h"
#include "StaticMesh.h"
#include "utility.h"
#include "Game.h"
#include "Collision.h"

using namespace std;
using namespace DirectX::SimpleMath;

// コンストラクタ
Pole::Pole(
	const DirectX::SimpleMath::Vector3& pos,
	const DirectX::SimpleMath::Vector3& size)
	:ColliderObject(pos, size)
{

}

// デストラクタ
Pole::~Pole()
{

}

//=======================================
// 初期化処理
//=======================================
void Pole::Init()
{
	// メッシュ読み込み
	StaticMesh staticmesh;

	// 3Dモデルデータ
	std::u8string modelFile = u8"assets/model/golf_pole/golf_pole_NO_HOLE.obj";

	// テクスチャディレクトリ
	std::string texDirectory = "assets/model/golf_pole";

	// Meshを読み込む
	std::string tmpStr1(reinterpret_cast<const char*>(modelFile.c_str()), modelFile.size());
	staticmesh.Load(tmpStr1, texDirectory);

	m_MeshRenderer.Init(staticmesh);

	//当たり判定用のサイズを設定
	collider.size = GetScale() * (staticmesh.GetMax() - staticmesh.GetMin());
	collider.size.x *= 0.25;

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
void Pole::Update()
{
	collider.center = m_Position;
}

//=======================================
// 描画処理
//=======================================
void Pole::Draw()
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

#ifdef _DEBUG
	Matrix colliderWorldMatrix = r * t;
	collider.DrawDebugCollider(Game::GetInstance().GetMainCamera(), worldmtx);
#endif
}

//=======================================
// 終了処理
//=======================================
void Pole::Uninit()
{

}


