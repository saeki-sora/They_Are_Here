#include "FadeTransition.h"
#include "Renderer.h"
#include <algorithm>

Shader FadeTransition::m_Shader;
ComPtr<ID3D11Buffer> FadeTransition::m_VertexBuffer;
ComPtr<ID3D11Buffer> FadeTransition::m_ConstantBuffer;

struct CbFade
{
	float fadeAlpha;
	DirectX::SimpleMath::Vector3 padding;
};

// ================================================================================
// FadeTransition実装
// ================================================================================
FadeTransition::FadeTransition(float duration, const DirectX::SimpleMath::Color& color)
    : m_Duration(duration), m_FadeColor(color)
{
}

void FadeTransition::InitResources()
{
    // 既に初期化済みなら何もしない
    if (m_Shader.IsValid()) return;

    //ShaderManager経由でシェーダーを作成
    m_Shader.Create("shader/VS_Fade.hlsl", "shader/PS_Fade.hlsl");

    //画面全体を覆う四角形の頂点データを作成
    VERTEX_3D vertices[] = {
        { {-1.0f,  1.0f, 0.0f}, {}, {1,1,1,1}, {0.0f, 0.0f} },
        { { 1.0f,  1.0f, 0.0f}, {}, {1,1,1,1}, {1.0f, 0.0f} },
        { {-1.0f, -1.0f, 0.0f}, {}, {1,1,1,1}, {0.0f, 1.0f} },
        { { 1.0f, -1.0f, 0.0f}, {}, {1,1,1,1}, {1.0f, 1.0f} },
    };

    //頂点バッファ作成
    D3D11_BUFFER_DESC bd = {};
    bd.ByteWidth = sizeof(vertices);
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA sd = {};
    sd.pSysMem = vertices;
    Renderer::GetDevice()->CreateBuffer(&bd, &sd, m_VertexBuffer.GetAddressOf());

    //定数バッファ作成
    if (!m_ConstantBuffer) {
        D3D11_BUFFER_DESC bd = {};
        bd.ByteWidth = sizeof(CbFade);
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

        Renderer::GetDevice()->CreateBuffer(&bd, nullptr, m_ConstantBuffer.GetAddressOf());
    }
}

void FadeTransition::Start()
{
    m_Phase = Phase::FadeOut;
    m_Timer = 0.0f;
    m_IsComplete = false;
}

void FadeTransition::Update(float deltaTime)
{
    m_Timer += deltaTime;

    switch (m_Phase) 
    {
    case Phase::FadeOut:
    case Phase::FadeIn:
        if (m_Timer >= m_Duration)
        {
            m_Timer = m_Duration;
            if (m_Phase == Phase::FadeIn) 
            {
                m_IsComplete = true;
            }
        }
        break;
    case Phase::Loading:
        // ローディング中は特に処理なし
        break;
    }
}

void FadeTransition::Draw()
{
    InitResources();

    float alpha = 0.0f;
    switch (m_Phase)
    {
    case Phase::FadeOut: alpha = m_Timer / m_Duration; break;
    case Phase::Loading: alpha = 1.0f; break;
    case Phase::FadeIn: alpha = 1.0f - (m_Timer / m_Duration); break;
    }

    alpha = std::clamp(alpha, 0.0f, 1.0f);

    //シェーダーをセット
    m_Shader.SetGPU();


    //定数バッファを更新
    CbFade data = {};
    data.fadeAlpha = alpha; // 計算したアルファ値を直接設定
    Renderer::GetDeviceContext()->UpdateSubresource(m_ConstantBuffer.Get(), 0, nullptr, &data, 0, 0);
    Renderer::GetDeviceContext()->PSSetConstantBuffers(0, 1, m_ConstantBuffer.GetAddressOf());

    //ブレンドモードと描画
    Renderer::SetBlendState(BS_ALPHABLEND);
    UINT stride = sizeof(VERTEX_3D);
    UINT offset = 0;
    Renderer::GetDeviceContext()->IASetVertexBuffers(0, 1, m_VertexBuffer.GetAddressOf(), &stride, &offset);
    Renderer::GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    Renderer::GetDeviceContext()->Draw(4, 0);
    Renderer::SetBlendState(BS_NONE);
}

float FadeTransition::GetProgress() const 
{
    if (m_Duration <= 0.0f) return 1.0f;
    return std::clamp(m_Timer / m_Duration, 0.0f, 1.0f);
}