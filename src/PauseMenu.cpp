#include "pch.h"
#include "PauseMenu.h"
#include "VisualObject.h"
#include "input.h"
#include "Application.h"

void PauseMenu::Init()
{
    m_Background = std::make_unique<VisualObject>();
    m_Background->Init();
    m_Background->SetTexture("assets/texture/Option.png");
}

void PauseMenu::Uninit()
{
    if (m_Background) { m_Background->Uninit(); m_Background.reset(); }
}

PauseAction PauseMenu::Update()
{
    if (!m_IsActive)
    {
        // 非アクティブ時: ESC でオープン
        if (Input::GetKeyTrigger(VK_ESCAPE))
        {
            m_IsActive = true;
            return PauseAction::Open;
        }
        return PauseAction::None;
    }

    // アクティブ時: ESC でクローズ、R でタイトルへ
    if (Input::GetKeyTrigger(VK_ESCAPE))
    {
        m_IsActive = false;
        return PauseAction::Close;
    }

    if (Input::GetKeyTrigger('R'))
    {
        m_IsActive = false;
        return PauseAction::ReturnToTitle;
    }

    return PauseAction::None;
}

void PauseMenu::Draw()
{
    if (!m_IsActive) return;

    const float screenW = (float)Application::GetWidth();
    const float screenH = (float)Application::GetHeight();
    const float scale   = 0.85f;

    m_Background->SetScale(screenW * scale, screenH * scale, 1.0f);
    m_Background->SetPosition(0.0f, 0.0f, 0.0f);
    m_Background->Draw();
}
