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
    PauseAction Update();
    void Draw();

    bool IsActive() const { return m_IsActive; }

private:
    bool m_IsActive = false;
    std::unique_ptr<VisualObject> m_Background;
};
