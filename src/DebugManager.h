#pragma once
#include "Input.h"

class DebugManager
{
private:
    bool m_DebugModeEnabled;          // デバッグモード全体のON/OFF
    bool m_ShowColliders;             // コライダーの表示
    bool m_NoClipMode;                // 飛行モード(壁すり抜け)
    bool m_ShowEnemyPath;             // 敵の経路表示
    bool m_ShowEnemyVision;           // 敵の視界表示
    bool m_ShowEnemyWhisker;          // 敵のウィスカー表示
    bool m_InvincibleMode;            // 無敵モード
    bool m_ShowLampLights;            // ランプ点光源の位置表示

    float m_KeyComboTimer;            // キー組み合わせ検出用タイマー
    static constexpr float COMBO_TIMEOUT = 0.5f; // 組み合わせ入力の制限時間

    int m_ComboStep;                  // 現在のコンボステップ

    DebugManager();

public:
    ~DebugManager();

    static DebugManager& GetInstance();

    void Update(float deltaTime);

    // デバッグモード全体の切り替え
    void ToggleDebugMode();

    // 個別機能の切り替え
    void ToggleColliderDisplay();
    void ToggleNoClipMode();
    void ToggleEnemyPathDisplay();
    void ToggleEnemyVisionDisplay();
    void ToggleInvincibleMode();
    void ToggleEnemyWhiskerDisplay();
    void ToggleLampLightsDisplay();

    // 状態取得
    bool IsDebugModeEnabled() const { return m_DebugModeEnabled; }
    bool ShouldShowColliders() const { return m_DebugModeEnabled && m_ShowColliders; }
    bool IsNoClipMode() const { return m_DebugModeEnabled && m_NoClipMode; }
    bool ShouldShowEnemyPath() const { return m_DebugModeEnabled && m_ShowEnemyPath; }
    bool ShouldShowEnemyVision() const { return m_DebugModeEnabled && m_ShowEnemyVision; }
    bool ShouldShowEnemyWhisker() const { return m_DebugModeEnabled && m_ShowEnemyWhisker; }
    bool IsInvincibleMode() const { return m_DebugModeEnabled && m_InvincibleMode; }
    bool ShouldShowLampLights() const { return m_DebugModeEnabled && m_ShowLampLights; }
};