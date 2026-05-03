#include "pch.h"
#include "Block.h"
#include "StaticMesh.h"
#include "utility.h"
#include "Game.h"
#include "DebugManager.h"
#include"TextureManager.h"

using namespace std;
using namespace DirectX::SimpleMath;

// 静的メンバの実体定義
MeshRenderer                           Block::s_SharedRenderer;
bool                                   Block::s_MeshReady = false;
DirectX::SimpleMath::Vector3           Block::s_ColliderExtents;
std::vector<SUBSET>                    Block::s_SharedSubsets;
std::vector<MATERIAL>                  Block::s_SharedMaterials;
std::vector<std::shared_ptr<Texture>>  Block::s_NormalTextures;

//コンストラクタ
Block::Block(
	const DirectX::SimpleMath::Vector3& pos,
	const DirectX::SimpleMath::Vector3& size,
	BlockType type)
	:ColliderObject( pos, size), m_type(type)
{
	m_Rotation = { 0.0f, 0.0f, 0.0f };
}

//デストラクタ
Block::~Block(){}

//=======================================
// 初期化
//=======================================
void Block::Init()
{
	// FBXモデルは全インスタンスで共有するため、最初の1回だけロードしてGPUバッファを作成
	if (!s_MeshReady)
	{
		StaticMesh staticmesh;
		std::u8string modelFile = u8"assets/model/Block/Block.fbx";
		std::string tmpStr1(reinterpret_cast<const char*>(modelFile.c_str()), modelFile.size());
		staticmesh.Load(tmpStr1, "assets/model/Block");

		s_SharedRenderer.Init(staticmesh);
		s_ColliderExtents  = staticmesh.GetMax() - staticmesh.GetMin();
		s_SharedSubsets    = staticmesh.GetSubsets();
		s_SharedMaterials  = staticmesh.GetMaterials();
		s_NormalTextures   = staticmesh.GetTextures();
		s_MeshReady = true;
	}

	// 共有データからコピー
	collider.size = GetScale() * s_ColliderExtents;
	m_subsets  = s_SharedSubsets;
	m_Textures = s_NormalTextures; // shared_ptrコピー

	// シェーダ
	m_Shader.Create("shader/litTextureVS.hlsl", "shader/litTexturePS.hlsl");

	// タイプ別テクスチャ上書き
	std::string overrideTexPath;
	switch (m_type)
	{
	case BlockType::FarGoal:
		overrideTexPath = "assets/texture/SpeedAreaBlock.png";
		break;
	case BlockType::MidGoal:
		overrideTexPath = "assets/texture/MidSpeedAreaBlock.png";
		break;
	default:
		break;
	}
	if (!overrideTexPath.empty())
	{
		auto customTex = TextureManager::GetInstance().Load(overrideTexPath);
		for (auto& tex : m_Textures) tex = customTex;
	}

	// マテリアル生成（IOなし、高速）
	for (int i = 0; i < (int)s_SharedMaterials.size(); ++i)
	{
		MATERIAL mat = s_SharedMaterials[i];
		mat.TextureEnable = true;
		std::unique_ptr<Material> m = std::make_unique<Material>();
		m->Create(mat);
		m_Materiales.push_back(std::move(m));
	}
}

//=======================================
// 更新処理
//=======================================
void Block::Update(float deltaTime)
{

}

//=======================================
// 描画処理
//=======================================
void Block::Draw()
{
	// ワールド行列を計算
	Matrix r = Matrix::CreateFromYawPitchRoll(m_Rotation.y, m_Rotation.x, m_Rotation.z);
	Matrix t = Matrix::CreateTranslation(m_Position.x, m_Position.y, m_Position.z);
	Matrix s = Matrix::CreateScale(m_Scale.x, m_Scale.y, m_Scale.z);

	Matrix worldmtx;
	worldmtx = s * r * t;
	Renderer::SetWorldMatrix(&worldmtx); // GPUにセット

	m_Shader.SetGPU();

	Renderer::BindLitCommonCB();

	// インデックスバッファ・頂点バッファをセット（共有GPUバッファ使用）
	s_SharedRenderer.BeforeDraw();

	//マテリアル数分ループ
	for (int i = 0; i < m_subsets.size(); ++i)
	{
		// マテリアルをセット
		m_Materiales[m_subsets[i].MaterialIdx]->SetGPU();

		if (m_Materiales[m_subsets[i].MaterialIdx]->isTextureEnable())
		{
			m_Textures[m_subsets[i].MaterialIdx]->SetGPU();
		}

		s_SharedRenderer.DrawSubset(
			m_subsets[i].IndexNum,
			m_subsets[i].IndexBase,
			m_subsets[i].VertexBase);
	}

	if (DebugManager::GetInstance().ShouldShowColliders())
	{
		collider.DrawDebugCollider(Game::GetInstance().GetMainCamera());
	}
}

// シャドウマップ描画
void Block::DrawShadow()
{
	// ワールド行列を計算
	Matrix r = Matrix::CreateFromYawPitchRoll(m_Rotation.y, m_Rotation.x, m_Rotation.z);
	Matrix t = Matrix::CreateTranslation(m_Position.x, m_Position.y, m_Position.z);
	Matrix s = Matrix::CreateScale(m_Scale.x, m_Scale.y, m_Scale.z);
	Matrix worldmtx = s * r * t;

	Renderer::SetWorldMatrix(&worldmtx);
	Renderer::SetShadowStaticShader();
	s_SharedRenderer.BeforeDraw();

	for (int i = 0; i < m_subsets.size(); ++i)
	{
		s_SharedRenderer.DrawSubset(
			m_subsets[i].IndexNum,
			m_subsets[i].IndexBase,
			m_subsets[i].VertexBase);
	}
}

//=======================================
// 終了処理
//=======================================
void Block::Uninit(){}
