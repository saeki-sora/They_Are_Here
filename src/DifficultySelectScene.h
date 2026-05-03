#pragma once
#include "SceneBase.h"
#include "DifficultyManager.h"

class VisualObject;

// ================================================================================
// DifficultySelectScene
//
// タイトルの「Game Start」選択後に表示される難易度選択画面。
//   ・EasyUI.png / MediumUI.png / HardUI.png を横並びに表示する
//   ・A/D（または左右矢印キー）で選択肢を移動
//   ・Space で決定 → DifficultyManager に選択を通知してゲームへ遷移
//
// ミニマップ・エフェクト等は存在せず、UIオブジェクトのみで構成するシンプルなシーン。
// ================================================================================

class DifficultySelectScene : public SceneBase
{
private:

    // 背景画像
    std::unique_ptr<VisualObject> m_Background;

    // 各難易度の UI 画像オブジェクト（VisualObject を直接管理）
    std::unique_ptr<VisualObject> m_Panels[DifficultyManager::DIFFICULTY_COUNT];

    // 現在カーソルが当たっている難易度インデックス（0=Easy, 1=Medium, 2=Hard）
    int m_SelectedIndex = 1;   // デフォルトは Medium

    // 入力の連打防止フラグ
    bool m_InputEnabled = true;

    // 決定アニメーション用タイマー（決定後しばらく待ってからシーン遷移する）
    bool  m_IsDecided      = false;
    float m_DecideTimer    = 0.0f;
    static constexpr float DECIDE_DELAY     = 1.0f;  // 決定後 1.0 秒待ってから遷移
    static constexpr float DECIDE_SCALE_MAX = 1.5f;  // 決定演出での最大拡大率

    // UI レイアウト設定
    static constexpr float PANEL_W       = 380.0f; // 各パネルの幅
    static constexpr float PANEL_H       = 500.0f; // 各パネルの高さ
    static constexpr float PANEL_GAP     = 50.0f;  // パネル間の隙間
    static constexpr float SELECTED_SCALE = 1.15f;  // 選択中パネルの拡大率

    // 各パネルの X 位置を計算するヘルパー
    float GetPanelX(int index) const;

    // 現在の選択状態を見た目に反映する
    void UpdatePanelVisuals();

    // 決定後のアニメーションを更新する（t: 0.0→1.0）
    void UpdateDecideAnimation(float t);

public:

    DifficultySelectScene()  = default;
    ~DifficultySelectScene() = default;

    void OnInit()   override;
    void OnUpdate(float deltaTime) override;
    void OnDrawUI() override;
    void OnUnload() override;
};
