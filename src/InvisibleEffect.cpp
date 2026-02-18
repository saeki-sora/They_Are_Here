#include "pch.h"
#include "InvisibleEffect.h"

InvisibleEffect::InvisibleEffect(float duration)
    : m_Timer(0.0f), m_Duration(duration), m_ConstantBuffer(nullptr)
{
    // --- 画面板の初期化 ---
    m_ScreenPoly.Init();
    m_ScreenPoly.SetPosition(DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f));
    m_ScreenPoly.SetScale(DirectX::SimpleMath::Vector3(1.0f, 1.0f, 1.0f));

    m_ScreenPoly.SetShader("shader/InvisibleEffectVS.hlsl", "shader/InvisibleEffectPS.hlsl");

    // --- 定数バッファ作成 ---
    D3D11_BUFFER_DESC bd = {};
    bd.ByteWidth = sizeof(InvisibleConstantBuffer);
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = 0;
    Renderer::GetDevice()->CreateBuffer(&bd, nullptr, &m_ConstantBuffer);
}

InvisibleEffect::~InvisibleEffect()
{
    if (m_ConstantBuffer)
    {
        m_ConstantBuffer->Release();
        m_ConstantBuffer = nullptr;
    }
}

void InvisibleEffect::Update(float deltaTime)
{
    // 時間を進める
    m_Timer += 1.0f / 60.0f;
}

void InvisibleEffect::Draw()
{
    if (!m_ConstantBuffer) return;

    // シェーダーに時間を送る
    InvisibleConstantBuffer cb;
    cb.time = m_Timer;
    cb.totalTime = m_Duration;
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

bool InvisibleEffect::IsPlaying() const
{
    // 時間切れになったら終了
    return m_Timer < m_Duration;
}