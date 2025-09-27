//#include <memory>
//#include "Arrow.h"
//#include "StaticMesh.h"
//#include "utility.h"
//#include "Game.h"
////#include "GolfBall.h"
//
//using namespace std;
//using namespace DirectX::SimpleMath;
//
//// コンストラクタ
//Arrow::Arrow(
//	const DirectX::SimpleMath::Vector3& pos ,
//	const DirectX::SimpleMath::Vector3& size) :VisualObject(pos,size)
//{
//
//}
//
//// デストラクタ
//Arrow::~Arrow()
//{
//
//}
//
////=======================================
//// 初期化処理
////=======================================
//void Arrow::Init()
//{
//	// メッシュ読み込み
//	StaticMesh staticmesh;
//
//	// 3Dモデルデータ
//	std::u8string modelFile = u8"assets/model/arrow/arrow.fbx";
//
//	// テクスチャディレクトリ
//	std::string texDirectory = "assets/model/arrow";
//
//	// Meshを読み込む
//	std::string tmpStr1(reinterpret_cast<const char*>(modelFile.c_str()), modelFile.size());
//	staticmesh.Load(tmpStr1, texDirectory);
//
//	m_MeshRenderer.Init(staticmesh);
//
//	// シェーダオブジェクト生成
//	m_Shader.Create("shader/litTextureVS.hlsl", "shader/litTexturePS.hlsl");
//
//	// サブセット情報取得
//	m_subsets = staticmesh.GetSubsets();
//
//	// テクスチャ情報取得
//	m_Textures = staticmesh.GetTextures();
//
//	// マテリアル情報取得
//	vector<MATERIAL> materials = staticmesh.GetMaterials();
//
//	// マテリアル数分ループ
//	for (int i = 0; i < materials.size(); i++)
//	{
//		// マテリアルオブジェクト生成
//		std::unique_ptr<Material> m = std::make_unique<Material>();
//
//		// マテリアル情報をセット
//		m->Create(materials[i]);
//
//		// マテリアルオブジェクトを配列に追加
//		m_Materiales.push_back(std::move(m));
//	}
//
//	// モデルによってスケールを調整
//	m_Scale.x = 3;
//	m_Scale.y = 3;
//	m_Scale.z = 3;
//
//	m_State = 1;
//}
//
////=======================================
//// 更新処理
////=======================================
//void Arrow::Update()
//{
//	if (m_State == 0)return; // 非表示ならreturn
//
//	// ゴルフボールの位置を取得
//	//vector<GolfBall*> ballpt = Game::GetInstance()->GetObjects<GolfBall>();
//	//if (ballpt.size() > 0)
//	//{
//	//	// 矢印の位置を更新
//	//	m_Position = ballpt[0]->GetPosition();
//	//}
//
//	// 方向選択なら
//	if (m_State == 1)
//	{
//		m_Scale.z = 3; // 長さを固定
//
//		// 向きを回転させる
//		m_Rotation.y += 0.03f;
//
//		if (m_Rotation.y > 6.28)m_Rotation.y = 0;
//	}
//	// パワー選択なら
//	else if (m_State == 2)
//	{
//		// 大きさを変更させる
//		m_Scale.z += 0.04f;
//		if (m_Scale.z > 4)m_Scale.z = 1;
//	}
//}
//
////=======================================
//// 描画処理
////=======================================
//void Arrow::Draw()
//{
//
//	// SRT情報作成
//	Matrix r = Matrix::CreateFromYawPitchRoll(m_Rotation.y, m_Rotation.x, m_Rotation.z);
//	Matrix t = Matrix::CreateTranslation(m_Position.x, m_Position.y, m_Position.z);
//	Matrix s = Matrix::CreateScale(m_Scale.x, m_Scale.y, m_Scale.z);
//
//	Matrix worldmtx;
//	worldmtx = s * r * t;
//	Renderer::SetWorldMatrix(&worldmtx); // GPUにセット
//
//	m_Shader.SetGPU();
//
//	// インデックスバッファ・頂点バッファをセット
//	m_MeshRenderer.BeforeDraw();
//
//	//マテリアル数分ループ 
//	for (int i = 0; i < m_subsets.size(); i++)
//	{
//		// マテリアルをセット(サブセット情報の中にあるマテリアルインデックスを使用)
//		m_Materiales[m_subsets[i].MaterialIdx]->SetGPU();
//
//		if (m_Materiales[m_subsets[i].MaterialIdx]->isTextureEnable())
//		{
//			m_Textures[m_subsets[i].MaterialIdx]->SetGPU();
//		}
//
//		m_MeshRenderer.DrawSubset(
//			m_subsets[i].IndexNum, // 描画するインデックス数
//			m_subsets[i].IndexBase, // 最初のインデックスバッファの位置	
//			m_subsets[i].VertexBase); // 頂点バッファの最初から使用
//	}
//}
//
////=======================================
//// 終了処理
////=======================================
//void Arrow::Uninit()
//{
//
//}
//
//
////状態の設定
//void Arrow::SetState(int s)
//{
//	m_State = s;
//}
//
//// 矢印のベクトルを取得
//Vector3 Arrow::GetVector()
//{
//	//矢印の初期状態の向き
//	Vector3 res = { 0, 0, -1 };
//
//	// ベクトルを回転
//	Matrix r = Matrix::CreateFromYawPitchRoll(m_Rotation.y, m_Rotation.x, m_Rotation.z);
//	res = Vector3::Transform(res, r);
//
//	//矢印の長さ(パワー)を掛ける
//	res *= m_Scale.z;
//
//	return res;
//}
