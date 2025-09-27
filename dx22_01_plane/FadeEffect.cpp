#include "FadeEffect.h"
#include "Renderer.h"
#include "Application.h" // 画面サイズ取得のためにインクルード
#include <iostream>

// 静的メンバ変数の実体を定義
ComPtr<ID3D11Buffer>      FadeEffect::m_vertexBuffer;
ComPtr<ID3D11Buffer>      FadeEffect::m_constantBuffer;

Shader FadeEffect::m_shader;

// 定数バッファの構造体
struct CbFade
{
	float fadeAlpha;
	DirectX::SimpleMath::Vector3 padding;
};

FadeEffect::FadeEffect(float duration, bool isFadeIn)
	: m_duration(duration), m_isFadeIn(isFadeIn)
{

	std::cout << "[FadeEffect] Constructor called. Address: " << this << std::endl;
	InitResources();
	m_timer = isFadeIn ? m_duration : 0.0f;// フェードインなら最初は最大、フェードアウトなら最初は0
}

void FadeEffect::InitResources()
{
	//すでに初期化済みなら何もしない
	if (m_shader.IsValid()) return;

	m_shader.Create("shader/VS_Fade.hlsl", "shader/PS_Fade.hlsl");

	//画面全体を覆う四角形の頂点データを作成
	float w = static_cast<float>(Application::GetWidth());
	float h = static_cast<float>(Application::GetHeight());
	VERTEX_3D vertices[] = {
			{ {-1.0f,  1.0f, 0.0f}, {}, {1,1,1,1}, {0.0f, 0.0f} }, // 左上
			{ { 1.0f,  1.0f, 0.0f}, {}, {1,1,1,1}, {1.0f, 0.0f} }, // 右上
			{ {-1.0f, -1.0f, 0.0f}, {}, {1,1,1,1}, {0.0f, 1.0f} }, // 左下
			{ { 1.0f, -1.0f, 0.0f}, {}, {1,1,1,1}, {1.0f, 1.0f} }, // 右下
	};

	//頂点バッファ作成
	D3D11_BUFFER_DESC bd = {};
	bd.ByteWidth = sizeof(vertices);
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA sd = {};
	sd.pSysMem = vertices;

	Renderer::GetDevice()->CreateBuffer(&bd, &sd, m_vertexBuffer.GetAddressOf());

	//定数バッファ作成
	bd.ByteWidth = sizeof(CbFade);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	Renderer::GetDevice()->CreateBuffer(&bd, nullptr, m_constantBuffer.GetAddressOf());
}

void FadeEffect::Update()
{
	if (!m_isPlaying) return;

	const float deltaTime = 1.0f / 60.0f; // 固定フレームレートの場合

	if (m_isFadeIn)
	{
		m_timer -= deltaTime;
		if (m_timer <= 0.0f)
		{
			m_timer = 0.0f;
			m_isPlaying = false;
		}
	}
	else // FadeOut
	{
		m_timer += deltaTime;
		if (m_timer >= m_duration)
		{
			m_timer = m_duration;
			m_isPlaying = false;
		}
	}

	if (!m_isPlaying && m_onComplete)
	{
		m_onComplete();
		m_onComplete = nullptr;
	}
}

void FadeEffect::Draw()
{
	float alpha = m_timer / m_duration;
	if (alpha > 1.0f) alpha = 1.0f;

	if (alpha <= 0.0f) return;


	//このエフェクト専用のシェーダーをセット
	m_shader.SetGPU();

	//定数バッファを更新
	CbFade data;
	data.fadeAlpha = alpha;
	Renderer::GetDeviceContext()->UpdateSubresource(m_constantBuffer.Get(), 0, nullptr, &data, 0, 0);
	Renderer::GetDeviceContext()->PSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());

	//ブレンドモードを半透明合成に設定
	Renderer::SetBlendState(BS_ALPHABLEND);

	//頂点バッファをセットして描画
	UINT stride = sizeof(VERTEX_3D);
	UINT offset = 0;
	Renderer::GetDeviceContext()->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
	Renderer::GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	Renderer::GetDeviceContext()->Draw(4, 0);

	//ブレンドモードを元に戻す
	Renderer::SetBlendState(BS_NONE);
}

