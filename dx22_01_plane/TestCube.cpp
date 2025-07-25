#include	"TestCube.h"

using namespace DirectX::SimpleMath;

//=======================================
//初期化処理
//=======================================
void TestCube::Init()
{
	// 頂点データ
	std::vector<VERTEX_3D>	vertices;

	vertices.resize(24);


	// 上面 (Y軸正方向)
	vertices[0].position = Vector3(-10, 10, 10);  vertices[0].uv = Vector2(0, 0);
	vertices[1].position = Vector3(10, 10, 10);   vertices[1].uv = Vector2(0.33, 0);
	vertices[2].position = Vector3(-10, 10, -10); vertices[2].uv = Vector2(0, 0.5);
	vertices[3].position = Vector3(10, 10, -10);  vertices[3].uv = Vector2(0.33, 0.5);

	vertices[0].normal = Vector3(0, 1, 0);
	vertices[1].normal = Vector3(0, 1, 0);
	vertices[2].normal = Vector3(0, 1, 0);
	vertices[3].normal = Vector3(0, 1, 0);

	// 下面 (Y軸負方向)
	vertices[4].position = Vector3(-10, -10, 10); vertices[4].uv = Vector2(0.66, 0.5);
	vertices[5].position = Vector3(10, -10, 10);   vertices[5].uv = Vector2(1, 0.5);
	vertices[6].position = Vector3(-10, -10, -10); vertices[6].uv = Vector2(0.66, 1);
	vertices[7].position = Vector3(10, -10, -10);  vertices[7].uv = Vector2(1, 1);

	vertices[4].normal = Vector3(0, -1, 0);
	vertices[5].normal = Vector3(0, -1, 0);
	vertices[6].normal = Vector3(0, -1, 0);
	vertices[7].normal = Vector3(0, -1, 0);

	// 前面 (Z軸正方向)
	vertices[8].position = Vector3(-10, 10, 10); vertices[8].uv = Vector2(0.33, 0);
	vertices[9].position = Vector3(10, 10, 10);    vertices[9].uv = Vector2(0.66, 0);
	vertices[10].position = Vector3(-10, -10, 10); vertices[10].uv = Vector2(0.33, 0.5);
	vertices[11].position = Vector3(10, -10, 10); vertices[11].uv = Vector2(0.66, 0.5);

	vertices[8].normal = Vector3(0, 0, 1);
	vertices[9].normal = Vector3(0, 0, 1);
	vertices[10].normal = Vector3(0, 0, 1);
	vertices[11].normal = Vector3(0, 0, 1);

	// 背面 (Z軸負方向)
	vertices[12].position = Vector3(-10, 10, -10); vertices[12].uv = Vector2(0.33, 0.5);
	vertices[13].position = Vector3(10, 10, -10);  vertices[13].uv = Vector2(0.66, 0.5);
	vertices[14].position = Vector3(-10, -10, -10); vertices[14].uv = Vector2(0.33, 1);
	vertices[15].position = Vector3(10, -10, -10); vertices[15].uv = Vector2(0.66, 1);

	vertices[12].normal = Vector3(0, 0, -1);
	vertices[13].normal = Vector3(0, 0, -1);
	vertices[14].normal = Vector3(0, 0, -1);
	vertices[15].normal = Vector3(0, 0, -1);

	// 左面 (X軸負方向)
	vertices[16].position = Vector3(-10, 10, 10);  vertices[16].normal = Vector3(-1, 0, 0);
	vertices[17].position = Vector3(-10, 10, -10); vertices[17].normal = Vector3(-1, 0, 0);
	vertices[18].position = Vector3(-10, -10, 10); vertices[18].normal = Vector3(-1, 0, 0);
	vertices[19].position = Vector3(-10, -10, -10); vertices[19].normal = Vector3(-1, 0, 0);

	vertices[16].uv = Vector2(0.66, 0);
	vertices[17].uv = Vector2(1, 0);
	vertices[18].uv = Vector2(0.66, 0.5);
	vertices[19].uv = Vector2(1, 0.5);

	// 右面 (X軸正方向)
	vertices[20].position = Vector3(10, 10, 10);   vertices[20].normal = Vector3(1, 0, 0);
	vertices[21].position = Vector3(10, 10, -10);  vertices[21].normal = Vector3(1, 0, 0);
	vertices[22].position = Vector3(10, -10, 10);  vertices[22].normal = Vector3(1, 0, 0);
	vertices[23].position = Vector3(10, -10, -10); vertices[23].normal = Vector3(1, 0, 0);

	vertices[20].uv = Vector2(0, 0.5);
	vertices[21].uv = Vector2(0.33, 0.5);
	vertices[22].uv = Vector2(0, 1);
	vertices[23].uv = Vector2(0.33, 1);

	vertices[0].color = Color(1, 1, 1, 1);
	vertices[1].color = Color(1, 1, 1, 1);
	vertices[2].color = Color(1, 1, 1, 1);
	vertices[3].color = Color(1, 1, 1, 1);
	vertices[4].color = Color(1, 1, 1, 1);
	vertices[5].color = Color(1, 1, 1, 1);
	vertices[6].color = Color(1, 1, 1, 1);
	vertices[7].color = Color(1, 1, 1, 1);
	vertices[8].color = Color(1, 1, 1, 1);
	vertices[9].color = Color(1, 1, 1, 1);
	vertices[10].color = Color(1, 1, 1, 1);
	vertices[11].color = Color(1, 1, 1, 1);
	vertices[12].color = Color(1, 1, 1, 1);
	vertices[13].color = Color(1, 1, 1, 1);
	vertices[14].color = Color(1, 1, 1, 1);
	vertices[15].color = Color(1, 1, 1, 1);
	vertices[16].color = Color(1, 1, 1, 1);
	vertices[17].color = Color(1, 1, 1, 1);
	vertices[18].color = Color(1, 1, 1, 1);
	vertices[19].color = Color(1, 1, 1, 1);
	vertices[20].color = Color(1, 1, 1, 1);
	vertices[21].color = Color(1, 1, 1, 1);
	vertices[22].color = Color(1, 1, 1, 1);
	vertices[23].color = Color(1, 1, 1, 1);


	// 頂点バッファ生成
	m_VertexBuffer.Create(vertices);

	// インデックスバッファ生成
	std::vector<unsigned int> indices;
	indices.resize(36);

	indices = {

		// 上面
	0, 1, 2,
	1, 3, 2,
	// 下面
	4, 5, 6,
	5, 7, 6,
	// 前面
	8, 9, 10,
	9, 11, 10,
	// 背面
	12, 14, 13,
	13, 14, 15,
	// 左面
	16, 17, 18,
	17, 19, 18,
	// 右面
	20, 22, 21,
	21, 22, 23

	};

	// インデックスバッファ生成
	m_IndexBuffer.Create(indices);

	// シェーダオブジェクト生成
	//m_Shader.Create("shader/unlitTextureVS.hlsl", "shader/unlitTexturePS.hlsl");
	m_Shader.Create("shader/litTextureVS.hlsl", "shader/litTexturePS.hlsl");

	// テクスチャ読み込み
	bool sts= m_Texture.Load("assets/texture/dice.png");
	assert(sts == true);


}


//=======================================
//更新処理
//=======================================
void TestCube::Update()
{
	//回転させる
	m_Rotation.y += 0.01f;
	m_Rotation.x += 0.01f;

}

//=======================================
//描画処理
//=======================================
void TestCube::Draw()
{
	// SRT情報作成
	Matrix r = Matrix::CreateFromYawPitchRoll(m_Rotation.y, m_Rotation.x, m_Rotation.z);
	Matrix t = Matrix::CreateTranslation(m_Position.x, m_Position.y, m_Position.z);
	Matrix s = Matrix::CreateScale(m_Scale.x, m_Scale.y, m_Scale.z);

	Matrix worldmtx;
	worldmtx = s * r * t;
	Renderer::SetWorldMatrix(&worldmtx); // GPUにセット

	// 描画の処理
	ID3D11DeviceContext* devicecontext;
	devicecontext = Renderer::GetDeviceContext();

	// トポロジーをセット（プリミティブタイプ）
	devicecontext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	m_Shader.SetGPU();
	m_VertexBuffer.SetGPU();
	m_IndexBuffer.SetGPU();

	// テクスチャセット
	m_Texture.SetGPU();

	devicecontext->DrawIndexed(
		36,							// 描画するインデックス数（四角形なんで４）
		0,							// 最初のインデックスバッファの位置
		0);
}

//=======================================
//終了処理
//=======================================
void TestCube::Uninit()
{

}