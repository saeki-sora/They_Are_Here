#include "pch.h"
#include "FoundEffect.h"
#include "Renderer.h"

FoundEffect::FoundEffect()
    : m_Timer(0.0f), m_Intensity(1.0f), m_Duration(2.0f), m_ConstantBuffer(nullptr)
{
    // 画面全体を覆う板の作成
    m_ScreenPoly.Init();
    m_ScreenPoly.SetPosition(DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f));
    m_ScreenPoly.SetScale(DirectX::SimpleMath::Vector3(1.0f, 1.0f, 1.0f));

    m_ScreenPoly.SetShader("shader/InvisibleEffectVS.hlsl", "shader/FoundEffectPS.hlsl");

    // 定数バッファ作成
    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(FoundConstantBuffer);
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = 0;
    Renderer::GetDevice()->CreateBuffer(&bd, nullptr, &m_ConstantBuffer);
}

FoundEffect::~FoundEffect()
{
    if (m_ConstantBuffer)
    {
        m_ConstantBuffer->Release();
        m_ConstantBuffer = nullptr;
    }
}

// 更新処理
void FoundEffect::Update(float deltaTime)
{
    m_Timer += deltaTime;

    // 強度を下げてフェードアウト
    m_Intensity -= deltaTime * (1.0f / m_Duration);
}

// 描画処理
void FoundEffect::Draw()
{
    if (!m_ConstantBuffer) return;

    // 定数バッファ更新
    FoundConstantBuffer cb;
    cb.time = m_Timer;
    cb.intensity = m_Intensity;
    Renderer::GetDeviceContext()->UpdateSubresource(m_ConstantBuffer, 0, nullptr, &cb, 0, 0);
    Renderer::GetDeviceContext()->PSSetConstantBuffers(1, 1, &m_ConstantBuffer);

    // 描画設定
    Renderer::SetDepthEnable(false);
	Renderer::SetBlendState(BS_ADDITIVE);// 加算合成

    m_ScreenPoly.Draw();

    // 設定戻し
    Renderer::SetDepthEnable(true);
    Renderer::SetBlendState(BS_NONE);
}

// エフェクトが続いているか判定
bool FoundEffect::IsPlaying() const
{
    return m_Intensity > 0.0f;
}