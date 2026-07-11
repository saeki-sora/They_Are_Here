#include "pch.h"
#include "VisualObject.h"

using namespace DirectX::SimpleMath;

//=======================================
// コンストラクタ
//=======================================
VisualObject::VisualObject(const Vector3& pos, const Vector3& size)
    : Object(pos, size) // 基底クラスであるObjectのコンストラクタを呼び出す
{
}

//=======================================
// デストラクタ
//=======================================
VisualObject::~VisualObject()
{
}

//=======================================
// 初期化処理
//=======================================
void VisualObject::Init()
{
    // このクラスの標準的な形状として、テクスチャを貼るための四角形（クアッド）を生成します。
    // SkyDomeのような特殊な形状は、このInit()をオーバーライドして独自の頂点データを生成します。

    // 1. 頂点データを作成 (中心が原点の1x1の四角形)
    std::vector<VERTEX_3D> vertices(4);
    vertices[0].position = Vector3(-0.5f, 0.5f, 0.0f); // 左上
    vertices[0].normal = Vector3(0.0f, 0.0f, -1.0f);
    vertices[0].color = Color(1, 1, 1, 1);
    vertices[0].uv = Vector2(0, 0);

    vertices[1].position = Vector3(0.5f, 0.5f, 0.0f); // 右上
    vertices[1].normal = Vector3(0.0f, 0.0f, -1.0f);
    vertices[1].color = Color(1, 1, 1, 1);
    vertices[1].uv = Vector2(1, 0);

    vertices[2].position = Vector3(-0.5f, -0.5f, 0.0f); // 左下
    vertices[2].normal = Vector3(0.0f, 0.0f, -1.0f);
    vertices[2].color = Color(1, 1, 1, 1);
    vertices[2].uv = Vector2(0, 1);

    vertices[3].position = Vector3(0.5f, -0.5f, 0.0f); // 右下
    vertices[3].normal = Vector3(0.0f, 0.0f, -1.0f);
    vertices[3].color = Color(1, 1, 1, 1);
    vertices[3].uv = Vector2(1, 1);

    m_BaseVertices = vertices; // SetAlphaで頂点色を書き換える際の元データとして保持
    m_VertexBuffer.Create(vertices);

    // 2. インデックスデータを作成
    std::vector<unsigned int> indices = { 0, 1, 2, 2, 1, 3 };
    m_IndexBuffer.Create(indices);

    // 3. シェーダーをShaderManagerから取得
    // (ここでは基本的なテクスチャ付きシェーダーを指定)
    m_Shader.Create("shader/unlitTextureVS.hlsl", "shader/unlitTexturePS.hlsl");

    // 4. マテリアルを作成
    m_Materiale = std::make_unique<Material>();
    MATERIAL mtrl;
    mtrl.Diffuse = Color(1, 1, 1, 1);
    mtrl.TextureEnable = true; // テクスチャを有効にする
    m_Materiale->Create(mtrl);
}

//=======================================
// 更新処理
//=======================================
void VisualObject::Update(float deltaTime)
{
    // VisualObjectの派生クラスで必要に応じて実装
}

//=======================================
// 描画処理
//=======================================
void VisualObject::Draw()
{
    Renderer::SetWorldViewProjection2D();
    Renderer::SetDepthEnable(false);

    // SRT（スケール・回転・移動）情報からワールド行列を作成
    Matrix s = Matrix::CreateScale(m_Scale);
    Matrix r = Matrix::CreateFromYawPitchRoll(m_Rotation.y, m_Rotation.x, m_Rotation.z);
    Matrix t = Matrix::CreateTranslation(m_Position);
    Matrix worldMtx = s * r * t;
    Renderer::SetWorldMatrix(&worldMtx);

    ID3D11DeviceContext* context = Renderer::GetDeviceContext();

    // プリミティブタイプを設定 (三角形リスト)
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // 各種バッファやシェーダーをGPUにセット
    m_Shader.SetGPU();
    m_VertexBuffer.SetGPU();
    m_IndexBuffer.SetGPU();
    m_Texture.SetGPU();
    m_Materiale->SetGPU();

    // UVアニメーション用の情報をRendererにセット
    float u = m_NumU - 1;
    float v = m_NumV - 1;
    float uw = 1.0f / m_SplitX;
    float vh = 1.0f / m_SplitY;
    Renderer::SetUV(u, v, uw, vh);

    Renderer::SetBlendState(BS_ALPHABLEND);

    // インデックスバッファの数だけ描画
    context->DrawIndexed(6, 0, 0); // 四角形は6つのインデックスで描画

    Renderer::SetBlendState(BS_NONE);

    Renderer::SetDepthEnable(true);
}

//=======================================
// 終了処理
//=======================================
void VisualObject::Uninit()
{
    // 必要に応じてリソース解放処理などを記述
}

//=======================================
// セッター関数
//=======================================

void VisualObject::SetTexture(const char* imgname)
{
    // VisualObjectはunlitTexturePSのみで描画され、ノーマルマップを参照しないため生成不要
    m_Texture.Load(imgname, false);
}

void VisualObject::SetShader(const char* vsPath, const char* psPath)
{
    m_Shader.Create(vsPath, psPath);
}

void VisualObject::SetPosition(const float& x, const float& y, const float& z)
{
    m_Position = Vector3(x, y, z);
}

void VisualObject::SetPosition(const Vector3& pos)
{
    // 基底クラスのセッターを呼び出す
    Object::SetPosition(pos);
}

void VisualObject::SetRotation(const float& x, const float& y, const float& z)
{
    // deg→radに変換して格納
    m_Rotation = Vector3(
        DirectX::XMConvertToRadians(x),
        DirectX::XMConvertToRadians(y),
        DirectX::XMConvertToRadians(z)
    );
}

void VisualObject::SetRotation(const Vector3& rot)
{
    m_Rotation = rot;
}

void VisualObject::SetScale(const float& x, const float& y, const float& z)
{
    m_Scale = Vector3(x, y, z);
}

void VisualObject::SetScale(const Vector3& scl)
{
    // 基底クラスのセッターを呼び出す
    Object::SetScale(scl);
}

void VisualObject::SetUV(const float& nu, const float& nv, const float& sx, const float& sy)
{
    m_NumU = nu;
    m_NumV = nv;
    m_SplitX = sx;
    m_SplitY = sy;
}

void VisualObject::SetAlpha(float alpha)
{
    m_Alpha = std::clamp(alpha, 0.0f, 1.0f);

    if (m_BaseVertices.empty()) return;

    // 頂点カラーのアルファ値だけ書き換えて頂点バッファへ反映
    std::vector<VERTEX_3D> vertices = m_BaseVertices;
    for (auto& v : vertices)
    {
        v.color.w = m_Alpha;
    }
    m_VertexBuffer.Modify(vertices);
}