#include "pch.h"
#include "PauseMenu.h"
#include "VisualObject.h"
#include "input.h"
#include "Application.h"

static float EaseInOutQuad(float t)
{
    return t < 0.5f ? 2.0f * t * t : 1.0f - powf(-2.0f * t + 2.0f, 2.0f) / 2.0f;
}

void PauseMenu::Init()
{
    m_TutorialBg = std::make_unique<VisualObject>();
    m_TutorialBg->Init();
    m_TutorialBg->SetTexture("assets/texture/Tutorial.png");

    m_OptionBg = std::make_unique<VisualObject>();
    m_OptionBg->Init();
    m_OptionBg->SetTexture("assets/texture/OperationGuide.png");

    m_CursorRight = std::make_unique<VisualObject>();
    m_CursorRight->Init();
    m_CursorRight->SetTexture("assets/texture/Cursor.png");
    m_CursorRight->SetScale(-250.0f, 200.0f, 1.0f);

    m_CursorLeft = std::make_unique<VisualObject>();
    m_CursorLeft->Init();
    m_CursorLeft->SetTexture("assets/texture/Cursor.png");
    m_CursorLeft->SetScale(250.0f, 200.0f, 1.0f);
}

void PauseMenu::Uninit()
{
    if (m_TutorialBg)  { m_TutorialBg->Uninit();  m_TutorialBg.reset();  }
    if (m_OptionBg)    { m_OptionBg->Uninit();    m_OptionBg.reset();    }
    if (m_CursorRight) { m_CursorRight->Uninit(); m_CursorRight.reset(); }
    if (m_CursorLeft)  { m_CursorLeft->Uninit();  m_CursorLeft.reset();  }
}

PauseAction PauseMenu::Update(float deltaTime)
{
    if (!m_IsActive)
    {
        if (Input::GetKeyTrigger(VK_ESCAPE))
        {
            m_IsActive       = true;
            m_CurrentTab     = PauseTab::Tutorial;
            m_SlideDir       = 0;
            m_SlideProgress  = 0.0f;
            m_CursorAnimTimer = 0.0f;
            return PauseAction::Open;
        }
        return PauseAction::None;
    }

    // スライド中はキー入力を無視して進捗だけ更新
    if (m_SlideDir != 0)
    {
        m_SlideProgress += deltaTime / SLIDE_DURATION;
        if (m_SlideProgress >= 1.0f)
        {
            m_SlideProgress = 0.0f;
            m_CurrentTab    = (m_SlideDir == 1) ? PauseTab::Option : PauseTab::Tutorial;
            m_SlideDir      = 0;
        }
        m_CursorAnimTimer += deltaTime;
        return PauseAction::None;
    }

    // カーソルアニメーション更新
    m_CursorAnimTimer += deltaTime;

    // キー処理
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

    if (m_CurrentTab == PauseTab::Tutorial && Input::GetKeyTrigger('D'))
    {
        m_SlideDir      = 1;
        m_SlideProgress = 0.0f;
    }
    else if (m_CurrentTab == PauseTab::Option && Input::GetKeyTrigger('A'))
    {
        m_SlideDir      = -1;
        m_SlideProgress = 0.0f;
    }

    return PauseAction::None;
}

void PauseMenu::Draw()
{
    if (!m_IsActive) return;

    const float screenW = (float)Application::GetWidth();
    const float screenH = (float)Application::GetHeight();

    // スライドオフセット計算
    float tutorialOffsetX = 0.0f;
    float optionOffsetX   = 0.0f;

    if (m_SlideDir != 0)
    {
        float t = EaseInOutQuad(m_SlideProgress);
        if (m_SlideDir == 1)
        {
            // Tutorial→Option: Tutorial左へ, Optionが右から来る
            tutorialOffsetX = -screenW * t;
            optionOffsetX   =  screenW * (1.0f - t);
        }
        else
        {
            // Option→Tutorial: Optionが右へ, Tutorialが左から来る
            tutorialOffsetX = -screenW * (1.0f - t);
            optionOffsetX   =  screenW * t;
        }
    }
    else
    {
        // 停止中: 表示中のタブだけ中央、もう一方は画面外
        tutorialOffsetX = (m_CurrentTab == PauseTab::Tutorial) ?  0.0f : -screenW;
        optionOffsetX   = (m_CurrentTab == PauseTab::Option)   ?  0.0f :  screenW;
    }

    // Tutorial背景（全画面）
    m_TutorialBg->SetScale(screenW, screenH, 1.0f);
    m_TutorialBg->SetPosition(tutorialOffsetX, 0.0f, 0.0f);
    m_TutorialBg->Draw();

    // Option背景（全画面）
    m_OptionBg->SetScale(screenW, screenH, 1.0f);
    m_OptionBg->SetPosition(optionOffsetX, 0.0f, 0.0f);
    m_OptionBg->Draw();

    float bobY = sinf(m_CursorAnimTimer * 5.0f) * 10.0f;

    // Tutorial右端カーソル（Tutorialと同じXオフセット）
    m_CursorRight->SetPosition(screenW * 0.5f - 80.0f + tutorialOffsetX, bobY, 0.0f);
    m_CursorRight->Draw();

    // Option左端カーソル（Optionと同じXオフセット）
    m_CursorLeft->SetPosition(-screenW * 0.5f + 80.0f + optionOffsetX, bobY, 0.0f);
    m_CursorLeft->Draw();
}
