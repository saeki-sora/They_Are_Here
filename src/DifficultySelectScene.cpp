#include "pch.h"
#include "DifficultySelectScene.h"
#include "VisualObject.h"
#include "SceneManager.h"
#include "FadeTransition.h"
#include "Stage1Scene.h"
#include "Input.h"
#include "Game.h"
#include "SoundManager.h"
#include "Application.h"

// ================================================================================
// DifficultySelectScene::OnInit
// ================================================================================
void DifficultySelectScene::OnInit()
{
    Game::GetInstance().GetMainCamera().SetMode(Camera::Mode::Stop);

    static const char* textures[DifficultyManager::DIFFICULTY_COUNT] =
    {
        "assets/texture/EasyUI.png",
        "assets/texture/MediumUI.png",
        "assets/texture/HardUI.png",
    };

    // 背景
    m_Background = std::make_unique<VisualObject>();
    m_Background->Init();
    m_Background->SetTexture("assets/texture/SelectBack.png");
    m_Background->SetScale(
        static_cast<float>(Application::GetWidth()),
        static_cast<float>(Application::GetHeight()),
        1.0f);
    m_Background->SetPosition(0.0f, 0.0f, 0.0f);

    for (int i = 0; i < DifficultyManager::DIFFICULTY_COUNT; ++i)
    {
        m_Panels[i] = std::make_unique<VisualObject>();
        m_Panels[i]->Init();
        m_Panels[i]->SetTexture(textures[i]);
        m_Panels[i]->SetScale(PANEL_W, PANEL_H, 1.0f);
        m_Panels[i]->SetPosition(GetPanelX(i), 0.0f, 0.0f);
    }

    // デフォルト選択（前回のゲームから引き継ぐ）
    m_SelectedIndex = DifficultyManager::GetInstance().GetSelectedIndex();
    m_InputEnabled  = true;
    m_IsDecided     = false;
    m_DecideTimer   = 0.0f;

    UpdatePanelVisuals();
}



void DifficultySelectScene::OnUpdate(float deltaTime)
{
    // 決定後は演出アニメーションを処理してから遷移する
    if (m_IsDecided)
    {
        m_DecideTimer += deltaTime;
        float t = std::min(m_DecideTimer / DECIDE_DELAY, 1.0f);
        UpdateDecideAnimation(t);

        if (m_DecideTimer >= DECIDE_DELAY)
        {
            // フェードトランジション付きでゲームへ遷移
            SceneManager::GetInstance().ChangeScene<Stage1Scene>(
                std::make_unique<FadeTransition>(2.0f));
        }
        return;
    }

    if (!m_InputEnabled) return;

    const int count = DifficultyManager::DIFFICULTY_COUNT;

    // 左移動（A / 左矢印）
    if (Input::GetKeyTrigger('A') || Input::GetKeyTrigger(VK_LEFT))
    {
        m_SelectedIndex = (m_SelectedIndex - 1 + count) % count;
        UpdatePanelVisuals();
    }

    // 右移動（D / 右矢印）
    if (Input::GetKeyTrigger('D') || Input::GetKeyTrigger(VK_RIGHT))
    {
        m_SelectedIndex = (m_SelectedIndex + 1) % count;
        UpdatePanelVisuals();
    }

    // 決定（Space）
    if (Input::GetKeyTrigger(VK_SPACE))
    {
        // 選択した難易度を DifficultyManager に通知する
        // Stage1Scene::OnInit() は GetSelectedMap() / GetSelectedConfig() でこれを参照する
        DifficultyManager::GetInstance().SetSelectedIndex(m_SelectedIndex);

        m_InputEnabled = false;
        m_IsDecided    = true;
        m_DecideTimer  = 0.0f;
    }
}


void DifficultySelectScene::OnDrawUI()
{
    // 背景を最初に描画（パネルの奥に表示）
    if (m_Background)
    {
        m_Background->Draw();
    }

    for (int i = 0; i < DifficultyManager::DIFFICULTY_COUNT; ++i)
    {
        if (m_Panels[i])
        {
            m_Panels[i]->Draw();
        }
    }
}



void DifficultySelectScene::OnUnload()
{
    if (m_Background)
    {
        m_Background->Uninit();
        m_Background.reset();
    }

    for (auto& panel : m_Panels)
    {
        if (panel)
        {
            panel->Uninit();
            panel.reset();
        }
    }

    SceneBase::OnUnload();
}


// ================================================================================
// ヘルパー関数
// ================================================================================
float DifficultySelectScene::GetPanelX(int index) const
{
    // 3枚を左から均等配置。中央インデックス(1)が X=0 になるよう計算する。
    const float totalWidth = PANEL_W * DifficultyManager::DIFFICULTY_COUNT
                           + PANEL_GAP * (DifficultyManager::DIFFICULTY_COUNT - 1);
    const float startX = -totalWidth * 0.5f + PANEL_W * 0.5f;
    return startX + index * (PANEL_W + PANEL_GAP);
}




void DifficultySelectScene::UpdateDecideAnimation(float t)
{
    // EaseOut cubic でなめらかに
    const float te = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);

    // 選択パネル：中央へスライド＆拡大
    const float srcScale = PANEL_W * SELECTED_SCALE;
    const float dstScale = PANEL_W * DECIDE_SCALE_MAX;
    const float scaleX   = srcScale + (dstScale - srcScale) * te;
    const float scaleY   = PANEL_H * SELECTED_SCALE + (PANEL_H * DECIDE_SCALE_MAX - PANEL_H * SELECTED_SCALE) * te;
    const float srcX     = GetPanelX(m_SelectedIndex);
    const float posX     = srcX + (0.0f - srcX) * te;

    m_Panels[m_SelectedIndex]->SetScale(scaleX, scaleY, 1.0f);
    m_Panels[m_SelectedIndex]->SetPosition(posX, 0.0f, 0.0f);

    // 非選択パネル：縮んで消える
    for (int i = 0; i < DifficultyManager::DIFFICULTY_COUNT; ++i)
    {
        if (i == m_SelectedIndex) continue;
        if (!m_Panels[i]) continue;

        const float s = PANEL_W * (1.0f - te);
        const float sH = PANEL_H * (1.0f - te);
        m_Panels[i]->SetScale(s, sH, 1.0f);
    }
}


void DifficultySelectScene::UpdatePanelVisuals()
{
    for (int i = 0; i < DifficultyManager::DIFFICULTY_COUNT; ++i)
    {
        if (!m_Panels[i]) continue;

        float scaleX = PANEL_W;
        float scaleY = PANEL_H;

        // 選択中のパネルだけ拡大する
        if (i == m_SelectedIndex)
        {
            scaleX *= SELECTED_SCALE;
            scaleY *= SELECTED_SCALE;
        }

        m_Panels[i]->SetScale(scaleX, scaleY, 1.0f);
    }
}
