#include "pch.h"
#include "DashEffect.h"

DashEffect::DashEffect()
    : m_ConstantBuffer(nullptr)
{
    // 画面板の初期化
    m_ScreenPoly.Init();
    m_ScreenPoly.SetPosition(DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f));
    m_ScreenPoly.SetScale(DirectX::SimpleMath::Vector3(1.0f, 1.0f, 1.0f));

    // InvisibleEffectVS をそのまま再利用、PS は専用シェーダー
    m_ScreenPoly.SetShader("shader/InvisibleEffectVS.hlsl", "shader/DashEffectPS.hlsl");

    // 定数バッファ作成
    D3D11_BUFFER_DESC bd = {};
    bd.ByteWidth = sizeof(DashConstantBuffer);
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = 0;
    Renderer::GetDevice()->CreateBuffer(&bd, nullptr, &m_ConstantBuffer);
}

DashEffect::~DashEffect()
{
    if (m_ConstantBuffer)
    {
        m_ConstantBuffer->Release();
        m_ConstantBuffer = nullptr;
    }
}

void DashEffect::Update(float deltaTime)
{
    // 累積時間（ライン流れ用）
    m_Timer += deltaTime;

    // フェードイン / フェードアウト
    if (m_Active)
    {
        m_Alpha += FADE_IN_SPEED * deltaTime;
        if (m_Alpha > 1.0f) m_Alpha = 1.0f;
    }
    else
    {
        m_Alpha -= FADE_OUT_SPEED * deltaTime;
        if (m_Alpha < 0.0f) m_Alpha = 0.0f;
    }
}

void DashEffect::Draw()
{
    if (!m_ConstantBuffer || m_Alpha <= 0.001f) return;

    // シェーダーに時間とアルファを送る
    DashConstantBuffer cb;
    cb.time  = m_Timer;
    cb.alpha = m_Alpha;
    Renderer::GetDeviceContext()->UpdateSubresource(m_ConstantBuffer, 0, nullptr, &cb, 0, 0);
    Renderer::GetDeviceContext()->PSSetConstantBuffers(1, 1, &m_ConstantBuffer);

    // 描画設定（最前面＆半透明）
    Renderer::SetDepthEnable(false);
    Renderer::SetBlendState(BS_ALPHABLEND);

    m_ScreenPoly.Draw();

    // 設定戻し
    Renderer::SetDepthEnable(true);
    Renderer::SetBlendState(BS_NONE);
}

bool DashEffect::IsPlaying() const
{
    // アクティブか、フェードアウト中ならまだ生存
    return m_Active || m_Alpha > 0.001f;
}
