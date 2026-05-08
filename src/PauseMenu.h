#pragma once

class VisualObject;

// ポーズメニューの選択肢
enum class PauseAction
{
    None,
    Open,
    Close,
    ReturnToTitle,
};

class PauseMenu
{
public:
    void Init();
    void Uninit();
    PauseAction Update(float deltaTime);
    void Draw();

    bool IsActive() const { return m_IsActive; }

private:
    // 表示中のタブ
    enum class PauseTab { Tutorial, Option };
    PauseTab m_CurrentTab = PauseTab::Tutorial;

    // スライドアニメーション
    float m_SlideProgress = 0.0f;
    int   m_SlideDir      = 0;   // +1: Tutorial→Option, -1: Option→Tutorial, 0: 停止
    static constexpr float SLIDE_DURATION = 0.35f;

    // カーソルアニメーション
    float m_CursorAnimTimer = 0.0f;

    bool m_IsActive = false;

    std::unique_ptr<VisualObject> m_TutorialBg;   // Tutorial.png
    std::unique_ptr<VisualObject> m_OptionBg;     // Option.png
    std::unique_ptr<VisualObject> m_CursorRight;  // Cursor.png（Tutorial右端フワフワ）
    std::unique_ptr<VisualObject> m_CursorLeft;   // Cursor.png（Option左端フワフワ）
};
