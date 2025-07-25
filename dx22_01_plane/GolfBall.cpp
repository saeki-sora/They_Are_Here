//#include <memory>
//#include "GolfBall.h"
//#include "StaticMesh.h"
//#include "utility.h"
//#include"collision.h"
//#include"input.h"
//#include"Game.h"
//#include"Ground.h"
//#include"Pole.h"
//
//using namespace DirectX::SimpleMath;
//
////コンストラクタ
//GolfBall::GolfBall(Camera* cam) :Object(cam)
//{
//
//}
//
////デストラクタ
//GolfBall::~GolfBall()
//{
//
//}
//
//void GolfBall::Init()
//{
//	// メッシュ読み込み
//	StaticMesh staticmesh;
//
//	//3Dモデルデータ
//	std::u8string modelFile = u8"assets/model/golfball1/uploads_files_4439838_Golf+Ball_v1_001.fbx";
//
//	//テクスチャディレクトリ
//	std::string texDirectory = "assets/model/golfball1/Golf Ball_v1_001_Diffuse.png";
//
//	//Meshを読み込む
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
//	std::vector<MATERIAL> materials = staticmesh.GetMaterials();
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
//	//モデルによってスケールを調整
//	m_Scale.x = 1;
//	m_Scale.y = 1;
//	m_Scale.z = 1;
//
//	//速度を与える
//	m_Velocity.x = 3.0f;
//
//	m_Position = Vector3(0.0f, 30.0f, 0.0f);
//}
//
//void GolfBall::Update()
//{
//	if (m_State != 0)return;//静止状態なら処理しない
//
//	Vector3 oldPos = m_Position;//1フレーム前の位置を保存
//
//	if (m_Velocity.LengthSquared() < 0.03f)
//	{
//		m_StopCount++;
//	}
//	else
//	{
//		m_StopCount = 0;
//
//		float decelerationPower = 0.1f;
//
//		Vector3 deceleration = -m_Velocity;//逆方向に加速度を与える
//		deceleration.Normalize();//単位ベクトルに変換
//		m_Acceleration = deceleration * decelerationPower;//減速度を与える
//
//		m_Velocity += m_Acceleration;//速度に減速度を加算
//	}
//
//	//10フレーム以上静止状態が続いたら静止状態にする
//	if (m_StopCount > 10)
//	{
//		m_Velocity = Vector3(0.0f, 0.0f, 0.0f);
//		m_State = 1;//静止状態
//	}
//
//	//重力
//	const float gravity = 0.1f;
//	m_Velocity.y -= gravity;
//
//	//速度を座標に加算
//	m_Position += m_Velocity;
//
//	float radius = 10.0f;
//
//	//Groundの頂点情報を取得
//	std::vector<Ground*> grounds = Game::GetInstance()->GetObjects<Ground>();
//	std::vector<VERTEX_3D>vertices;
//
//	for (auto& g : grounds)
//	{
//		std::vector<VERTEX_3D>vecs = g->GetVertices();
//
//		for (auto& v : vecs)
//		{
//			vertices.push_back(v);
//		}
//	}
//
//	float moveDistance = 9999;//移動距離
//	Vector3 contactPoint;//接触点
//	Vector3 normal;//法線
//
//	//線分とポリゴンの当たり判定
//	bool senbunFg=false;
//
//	for (int i = 0; i < vertices.size(); i += 3)
//	{
//		Collision::Polygon collisionPolygon = {
//			vertices[i].position,
//			vertices[i + 1].position,
//			vertices[i + 2].position
//		};
//
//		Vector3 cp;
//		Collision::Segment collisionSegment = { oldPos, m_Position };
//
//		if (Collision::CheckHit(collisionSegment, collisionPolygon, cp))
//		{
//			float md = 0;
//			Vector3 np = Collision::moveSphere(collisionSegment, radius,collisionPolygon, cp,md);
//			if (md < moveDistance)
//			{
//				moveDistance = md;
//				m_Position = np;
//				contactPoint = cp;
//				normal = Collision::GetNormal(collisionPolygon);
//			}
//			senbunFg = true;
//		}
//	}
//
//	//地面との当たり判定
//	if (!senbunFg)
//	{
//		for (int i = 0; i < vertices.size(); i += 3)
//		{
//			Collision::Polygon collisionPolygon = {
//				vertices[i].position,
//				vertices[i + 1].position,
//				vertices[i + 2].position
//			};
//
//			Vector3 cp;
//			Collision::Sphere collisionSphere = { m_Position, radius };
//
//			if (Collision::CheckHit(collisionSphere, collisionPolygon, cp))
//			{
//				
//				float md = 0;
//				Vector3 np = Collision::moveSphere(collisionSphere, collisionPolygon, cp);
//				md = (np - oldPos).Length();
//				if (md < moveDistance)
//				{
//					moveDistance = md;
//					m_Position = np;
//					contactPoint = cp;
//					normal = Collision::GetNormal(collisionPolygon);
//				}
//			}
//		}
//	}
//	if (moveDistance != 9999)
//	{
//		m_Velocity.y = -gravity;
//
//		//反射ベクトル
//		float velocityNormal=Collision::Dot(m_Velocity, normal);//法線方向の速度
//		Vector3 v1=velocityNormal*normal;//法線方向の速度
//		Vector3 v2 = m_Velocity - v1;//法線方向の速度とそれ以外の速度に分ける
//
//		const float restitution = 0.8f;//反発係数
//	}
//
//	//下に落ちたらリスポーン
//	if (m_Position.y < -100)
//	{
//		m_Position = Vector3(0.0f, 50.0f, 0.0f);
//		m_Velocity= Vector3(0.0f, 0.0f, 0.0f);
//	}
//
//	//Poleの位置を取得
//	std::vector<Pole*> poles = Game::GetInstance()->GetObjects<Pole>();
//
//	if (poles.size() > 0)
//	{
//		Vector3 polePos = poles[0]->GetPosition();
//
//		//Poleとの当たり判定
//		Collision::Sphere balCollision = { m_Position, radius };
//		Collision::Sphere poleCollision = { polePos, 0.5f };
//
//		if (Collision::CheckHit(balCollision, poleCollision))
//		{
//			m_State = 2;//カップイン
//		}
//	}
//}
//
//void GolfBall::Draw()
//{
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
//	//カメラの情報をセット
//	m_Camera->SetCamera(0);
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
//			m_subsets[i].IndexNum,		// 描画するインデックス数
//			m_subsets[i].IndexBase,		// 最初のインデックスバッファの位置	
//			m_subsets[i].VertexBase);	// 頂点バッファの最初から使用
//	}
//}
//
//void GolfBall::Uninit()
//{
//
//}
//
////状態の設定・取得
//void GolfBall::SetState(int s) { m_State = s; }
//int GolfBall::GetState() { return m_State; }
//
////ショット
//void GolfBall::Shot(Vector3 v) { m_Velocity = v; }