#include "pch.h"
#include "KeyPickupNotice.h"

//-----------------------------------------------------------------------------
// 初期化処理
//-----------------------------------------------------------------------------
void KeyPickupNotice::Init()
{
    m_Icon = std::make_unique<VisualObject>();
    m_Icon->Init();
    m_Icon->SetTexture("assets/texture/GetKey_UI.png");
    m_Icon->SetScale(ICON_WIDTH, ICON_HEIGHT, 1.0f);
    m_Icon->SetPosition(ICON_POS_X, ICON_POS_Y, 0.0f);
    m_Icon->SetAlpha(0.0f);

    m_Phase = Phase::Idle;
    m_Timer = 0.0f;
}

//-----------------------------------------------------------------------------
// 通知の再生を開始する
//-----------------------------------------------------------------------------
void KeyPickupNotice::Play()
{
    m_Phase = Phase::FadeIn;
    m_Timer = 0.0f;
}

//-----------------------------------------------------------------------------
// 更新処理
//-----------------------------------------------------------------------------
void KeyPickupNotice::Update(float deltaTime)
{
    if (m_Phase == Phase::Idle) return;

    m_Timer += deltaTime;

    switch (m_Phase)
    {
    case Phase::FadeIn:

        if (m_Timer >= FADE_IN_DURATION)
        {
            m_Icon->SetAlpha(1.0f);
            m_Phase = Phase::Hold;
            m_Timer = 0.0f;
        }
        else
        {
            m_Icon->SetAlpha(m_Timer / FADE_IN_DURATION);
        }
        break;

    case Phase::Hold:

        if (m_Timer >= HOLD_DURATION)
        {
            m_Phase = Phase::FadeOut;
            m_Timer = 0.0f;
        }
        break;

    case Phase::FadeOut:

        if (m_Timer >= FADE_OUT_DURATION)
        {
            m_Icon->SetAlpha(0.0f);
            m_Phase = Phase::Idle;
            m_Timer = 0.0f;
        }
        else
        {
            m_Icon->SetAlpha(1.0f - (m_Timer / FADE_OUT_DURATION));
        }
        break;

    default:
        break;
    }
}

//-----------------------------------------------------------------------------
// 描画処理
//-----------------------------------------------------------------------------
void KeyPickupNotice::Draw()
{
    if (m_Phase == Phase::Idle) return;

    m_Icon->Draw();
}

//-----------------------------------------------------------------------------
// 終了処理
//-----------------------------------------------------------------------------
void KeyPickupNotice::Uninit()
{
    if (m_Icon)
    {
        m_Icon->Uninit();
        m_Icon.reset();
    }
}
