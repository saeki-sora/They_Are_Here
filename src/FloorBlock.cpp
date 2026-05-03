#include "pch.h"
#include "FloorBlock.h"
#include "StaticMesh.h"
#include "utility.h"
#include "Game.h"

using namespace std;
using namespace DirectX::SimpleMath;

// 静的メンバの実体定義
MeshRenderer                           FloorBlock::s_SharedRenderer;
bool                                   FloorBlock::s_MeshReady = false;
std::vector<SUBSET>                    FloorBlock::s_SharedSubsets;
std::vector<MATERIAL>                  FloorBlock::s_SharedMaterials;
std::vector<std::shared_ptr<Texture>>  FloorBlock::s_SharedTextures;

//コンストラクタ
FloorBlock::FloorBlock(
	const DirectX::SimpleMath::Vector3& pos,
	const DirectX::SimpleMath::Vector3& size)
	:VisualObject(pos, size)
{
}

// デストラクタ
FloorBlock::~FloorBlock() {}

//=======================================
// 初期化処理
//=======================================
void FloorBlock::Init()
{
	// FBXロードとGPUバッファ生成は初回のみ（全インスタンスで共有）
	if (!s_MeshReady)
	{
		StaticMesh staticmesh;
		std::u8string modelFile = u8"assets/model/floorBlock/FloorBlock.fbx";
		std::string tmpStr1(reinterpret_cast<const char*>(modelFile.c_str()), modelFile.size());
		staticmesh.Load(tmpStr1, "assets/model/floorBlock");

		s_SharedRenderer.Init(staticmesh);
		s_SharedSubsets   = staticmesh.GetSubsets();
		s_SharedMaterials = staticmesh.GetMaterials();
		s_SharedTextures  = staticmesh.GetTextures();
		s_MeshReady = true;
	}

	// 共有データからコピー（IOなし、高速）
	m_subsets  = s_SharedSubsets;
	m_Textures = s_SharedTextures; // shared_ptrコピー

	// シェーダ（ShaderManagerがキャッシュ済みなので2回目以降は高速）
	m_Shader.Create("shader/litTextureVS.hlsl", "shader/litTexturePS.hlsl");

	// マテリアル生成（IOなし、高速）
	for (int i = 0; i < (int)s_SharedMaterials.size(); ++i)
	{
		MATERIAL mat = s_SharedMaterials[i];
		std::unique_ptr<Material> m = std::make_unique<Material>();
		m->Create(mat);
		m_Materiales.push_back(std::move(m));
	}
}

//=======================================
// 更新処理
//=======================================
void FloorBlock::Update(float deltaTime)
{

}

//=======================================
// 描画処理
//=======================================
void FloorBlock::Draw()
{
	// SRT情報作成
	Matrix r = Matrix::CreateFromYawPitchRoll(m_Rotation.y, m_Rotation.x, m_Rotation.z);
	Matrix t = Matrix::CreateTranslation(m_Position.x, m_Position.y, m_Position.z);
	Matrix s = Matrix::CreateScale(m_Scale.x, m_Scale.y, m_Scale.z);

	Matrix worldmtx;
	worldmtx = s * r * t;
	Renderer::SetWorldMatrix(&worldmtx); // GPUにセット

	m_Shader.SetGPU();

	// インデックスバッファ・頂点バッファをセット（共有GPUバッファ使用）
	s_SharedRenderer.BeforeDraw();

	//マテリアル数分ループ
	for (int i = 0; i < m_subsets.size(); ++i)
	{
		// マテリアルをセット(サブセット情報の中にあるマテリアルインデックスを使用)
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

	//#ifdef _DEBUG
	//	Matrix colliderWorldMatrix = r * t;
	//	// スケールを含めない行列を渡して描画します。
	//	collider.DrawDebugCollider(Game::GetInstance().GetMainCamera(), colliderWorldMatrix);
	//#endif
}

// シャドウマップ描画
void FloorBlock::DrawShadow()
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
void FloorBlock::Uninit() {}
