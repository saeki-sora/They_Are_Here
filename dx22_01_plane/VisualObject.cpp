#include "VisualObject.h"

using namespace std;
using namespace DirectX::SimpleMath;

// コンストラクタ
VisualObject::VisualObject(
	const DirectX::SimpleMath::Vector3& pos,
	const DirectX::SimpleMath::Vector3& size) 
	:Object(pos,size)
{

}

// デストラクタ
VisualObject::~VisualObject()
{

}

//=======================================
// 初期化処理
//=======================================
void VisualObject::Init()
{
	// 頂点データ
	std::vector<VERTEX_3D> vertices;

	vertices.resize(4);

	vertices[0].position = Vector3(-0.5f, 0.5f, 0);
	vertices[1].position = Vector3(0.5f, 0.5f, 0);
	vertices[2].position = Vector3(-0.5f, -0.5f, 0);
	vertices[3].position = Vector3(0.5f, -0.5f, 0);

	vertices[0].color = Color(1, 1, 1, 1);
	vertices[1].color = Color(1, 1, 1, 1);
	vertices[2].color = Color(1, 1, 1, 1);
	vertices[3].color = Color(1, 1, 1, 1);

	vertices[0].uv = Vector2(0, 0);
	vertices[1].uv = Vector2(1, 0);
	vertices[2].uv = Vector2(0, 1);
	vertices[3].uv = Vector2(1, 1);

	// 頂点バッファ生成
	m_VertexBuffer.Create(vertices);

	// インデックスバッファ生成
	std::vector<unsigned int> indices;
	indices.resize(4);

	indices[0] = 0;
	indices[1] = 1;
	indices[2] = 2;
	indices[3] = 3;

	// インデックスバッファ生成
	m_IndexBuffer.Create(indices);

	// シェーダオブジェクト生成
	m_Shader.Create("shader/unlitTextureVS.hlsl", "shader/unlitTexturePS.hlsl");

	// マテリアル情報取得
	m_Materiale = std::make_unique<Material>();
	MATERIAL mtrl;
	mtrl.Diffuse = Color(1, 1, 1, 1);
	mtrl.Shiness = 1;
	mtrl.TextureEnable = true; // テクスチャを使うか否かのフラグ
	m_Materiale->Create(mtrl);
}

//=======================================
// 更新処理
//=======================================
void VisualObject::Update()
{

}

//=======================================
// 描画処理
//=======================================
void VisualObject::Draw()
{
	// SRT情報作成
	Matrix r = Matrix::CreateFromYawPitchRoll(m_Rotation.x, m_Rotation.y, m_Rotation.z);
	Matrix t = Matrix::CreateTranslation(m_Position.x, m_Position.y, m_Position.z);
	Matrix s = Matrix::CreateScale(m_Scale.x, m_Scale.y, m_Scale.z);

	Matrix worldmtx;
	worldmtx = s * r * t;
	Renderer::SetWorldMatrix(&worldmtx); // GPUにセット

	// 描画の処理
	ID3D11DeviceContext* devicecontext;
	devicecontext = Renderer::GetDeviceContext();

	// トポロジーをセット（プリミティブタイプ）
	devicecontext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	m_Shader.SetGPU();
	m_VertexBuffer.SetGPU();
	m_IndexBuffer.SetGPU();

	m_Texture.SetGPU();
	m_Materiale->SetGPU();

	// UVの設定を指定
	float u = m_NumU - 1;
	float v = m_NumV - 1;
	float uw = 1 / m_SplitX;
	float vh = 1 / m_SplitY;

	Renderer::SetUV(u, v, uw, vh);

	devicecontext->DrawIndexed(
		4, // 描画するインデックス数（四角形なんで４）
		0, // 最初のインデックスバッファの位置
		0);
}

//=======================================
// 終了処理
//=======================================
void VisualObject::Uninit()
{

}

// テクスチャを指定
void VisualObject::SetTexture(const char* imgname)
{
	// テクスチャロード
	bool sts = m_Texture.Load(imgname);
	assert(sts == true);
}

// 位置を指定
void VisualObject::SetPosition(const float& x, const float& y, const float& z)
{
	Vector3 p = { x, y, z };
	SetPosition(p);
}
void VisualObject::SetPosition(const Vector3& pos)
{
	m_Position = pos;
}

// 角度を指定
void VisualObject::SetRotation(const float& x, const float& y, const float& z)
{
	Vector3 r = { x, y, z };
	SetRotation(r);
}
void VisualObject::SetRotation(const Vector3& rot)
{
	m_Rotation = rot * 3.14f / 180; // deg→radに変換
}

// 大きさを指定
void VisualObject::SetScale(const float& x, const float& y, const float& z)
{
	Vector3 s = { x, y, z };
	SetScale(s);
}
void VisualObject::SetScale(const Vector3& scl)
{
	m_Scale = scl;
}

// UV座標を指定
void VisualObject::SetUV(const float& nu, const float& nv, const float& sx, const float& sy)
{
	m_NumU = nu;
	m_NumV = nv;
	m_SplitX = sx;
	m_SplitY = sy;
}
