#include "pch.h"
#include "SootObject.h"
#include "Renderer.h"
#include "Application.h"

using namespace DirectX::SimpleMath;

// 定数バッファ構造体 (HLSLに合わせる)
struct CbSoot
{
	Matrix wvp;
	Color color;
	float timer;
	Vector3 padding;
};

SootObject::SootObject(const Vector3& pos, const Vector3& size)
	: VisualObject(pos, size)
{
}

void SootObject::Init()
{
	// 基底クラスの初期化
	VisualObject::Init();

	// シェーダーをすす演出用に上書き
	m_Shader.Create("shader/SootTextVS.hlsl", "shader/SootTextPS.hlsl");

	// 定数バッファ作成
	D3D11_BUFFER_DESC bd = {};
	bd.ByteWidth = sizeof(CbSoot);
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	Renderer::GetDevice()->CreateBuffer(&bd, nullptr, m_ConstantBuffer.GetAddressOf());
}

void SootObject::StartAnim(float duration)
{
	m_Duration = duration;
	m_Timer = 0.0f;
	m_IsPlaying = true;
}

void SootObject::Update(float deltaTime)
{
	VisualObject::Update(deltaTime);

	if (m_IsPlaying)
	{
		// 0.0 -> 1.0 へ進める
		m_Timer += deltaTime / m_Duration;
		if (m_Timer >= 1.0f)
		{
			m_Timer = 1.0f;
			m_IsPlaying = false;
		}
	}
}

void SootObject::Draw()
{
    // 深度無効化など、2D描画の設定
    Renderer::SetDepthEnable(false);

    // 行列計算
    Matrix s = Matrix::CreateScale(m_Scale);
    Matrix r = Matrix::CreateFromYawPitchRoll(m_Rotation.y, m_Rotation.x, m_Rotation.z);
    Matrix t = Matrix::CreateTranslation(m_Position);
    Matrix world = s * r * t;

    // 2D用のプロジェクション行列（仮想解像度基準。実解像度が変わってもレイアウトを維持する）
    float w = (float)Application::VIRTUAL_WIDTH;
    float h = (float)Application::VIRTUAL_HEIGHT;
    Matrix proj = Matrix::CreateOrthographicOffCenter(0, w, 0, h, 0, 1.0f);

    // WVP行列作成
    Matrix wvp = world * proj;
    wvp = wvp.Transpose(); // HLSL用に転置

    // 定数バッファ更新
    CbSoot cb;
    cb.wvp = wvp;
    cb.color = m_Materiale ? m_Materiale->GetDiffuse() : Color(1, 1, 1, 1);
    cb.timer = m_Timer;
    Renderer::GetDeviceContext()->UpdateSubresource(m_ConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);

    // シェーダー・バッファセット
    m_Shader.SetGPU();
    Renderer::GetDeviceContext()->VSSetConstantBuffers(8, 1, m_ConstantBuffer.GetAddressOf());
    Renderer::GetDeviceContext()->PSSetConstantBuffers(8, 1, m_ConstantBuffer.GetAddressOf());

    m_Texture.SetGPU();

    m_VertexBuffer.SetGPU();
    m_IndexBuffer.SetGPU();

    // 描画実行
    Renderer::GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    Renderer::SetBlendState(BS_ALPHABLEND);
    Renderer::GetDeviceContext()->DrawIndexed(6, 0, 0);
    Renderer::SetBlendState(BS_NONE);

    Renderer::SetDepthEnable(true);
}