#pragma once
#include "VisualObject.h"

//-----------------------------------------------------------------------------
// KeyPickupNoticeクラス
// カギ取得時に画面下部へフェードイン→保持→フェードアウトで表示するUI通知
//-----------------------------------------------------------------------------
class KeyPickupNotice
{
public:
    void Init();
    void Update(float deltaTime);
    void Draw();
    void Uninit();

    // 通知の再生を開始する（カギ取得の瞬間に呼ぶ）
    void Play();

private:
    enum class Phase
    {
        Idle,     // 非表示
        FadeIn,   // フェードイン中
        Hold,     // 表示保持中
        FadeOut,  // フェードアウト中
    };

    static constexpr float FADE_IN_DURATION = 0.5f;  // フェードインにかかる秒数
    static constexpr float HOLD_DURATION = 2.0f;     // 表示を保持する秒数
    static constexpr float FADE_OUT_DURATION = 0.8f; // フェードアウトにかかる秒数

    static constexpr float ICON_WIDTH = 550.0f;  // アイコン幅（テクスチャのアスペクト比に合わせる）
    static constexpr float ICON_HEIGHT = 84.0f;  // アイコン高さ
    static constexpr float ICON_POS_X = 0.0f;    // 画面中央
    static constexpr float ICON_POS_Y = -300.0f; // 画面下部

    std::unique_ptr<VisualObject> m_Icon; // カギ取得UI画像
    Phase m_Phase = Phase::Idle;          // 現在のフェーズ
    float m_Timer = 0.0f;                 // 現フェーズの経過時間
};
