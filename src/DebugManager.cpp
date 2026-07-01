#include "pch.h"
#include "DebugManager.h"
#include <iostream>

DebugManager::DebugManager()
    : m_DebugModeEnabled(false)
    , m_ShowColliders(false)
    , m_NoClipMode(false)
    , m_ShowEnemyPath(false)
    , m_ShowEnemyVision(false)
    , m_ShowEnemyWhisker(false)
    , m_InvincibleMode(false)
    , m_ShowLampLights(false)
    , m_KeyComboTimer(0.0f)
    , m_ComboStep(0)
{
}

DebugManager::~DebugManager()
{
}

DebugManager& DebugManager::GetInstance()
{
    static DebugManager instance;
    return instance;
}

void DebugManager::Update(float deltaTime)
{
	if (Input::GetKeyTrigger('P'))// Pキーでデバッグモード全体の切り替え
    {
        ToggleDebugMode();
    }

    // デバッグモードが有効な場合のみ、個別機能の切り替えを受け付ける
    if (m_DebugModeEnabled)
    {
        // F1: コライダー表示切り替え
        if (Input::GetKeyTrigger(VK_F1))
        {
            ToggleColliderDisplay();
        }

        // F2: 飛行モード切り替え
        if (Input::GetKeyTrigger(VK_F2))
        {
            ToggleNoClipMode();
        }

        // F3: 敵の経路表示切り替え
        if (Input::GetKeyTrigger(VK_F3))
        {
            ToggleEnemyPathDisplay();
        }

        // F4: 敵の視界表示切り替え
        if (Input::GetKeyTrigger(VK_F4))
        {
            ToggleEnemyVisionDisplay();
        }

        // F5: 無敵モード切り替え
        if (Input::GetKeyTrigger(VK_F5))
        {
            ToggleInvincibleMode();
        }

        // F6: 敵のウィスカー表示切り替え
        if (Input::GetKeyTrigger(VK_F6))
        {
            ToggleEnemyWhiskerDisplay();
        }

        // F7: ランプ点光源の位置表示切り替え
        if (Input::GetKeyTrigger(VK_F7))
        {
            ToggleLampLightsDisplay();
        }
    }
}

void DebugManager::ToggleDebugMode()
{
    m_DebugModeEnabled = !m_DebugModeEnabled;

    if (m_DebugModeEnabled)
    {
#ifdef _DEBUG

        std::cout << "========================================\n";
        std::cout << "   DEBUG MODE ENABLED\n";
        std::cout << "========================================\n";
        std::cout << "F1: Toggle Collider Display\n";
        std::cout << "F2: Toggle No-Clip Mode\n";
        std::cout << "F3: Toggle Enemy Path Display\n";
        std::cout << "F4: Toggle Enemy Vision Display\n";
        std::cout << "F5: Toggle Invincible Mode\n";
        std::cout << "F6: Toggle Enemy Whisker Display\n";
        std::cout << "F7: Toggle Lamp Light Display\n";
        std::cout << "========================================\n";

#endif
    }
    else
    {
#ifdef _DEBUG

        std::cout << "DEBUG MODE DISABLED\n";

#endif

        // デバッグモードを無効にしたら、全ての機能もオフにする
        m_ShowColliders = false;
        m_NoClipMode = false;
        m_ShowEnemyPath = false;
        m_ShowEnemyVision = false;
        m_ShowEnemyWhisker = false;
        m_InvincibleMode = false;
        m_ShowLampLights = false;
    }
}

void DebugManager::ToggleColliderDisplay()
{
    m_ShowColliders = !m_ShowColliders;
}

void DebugManager::ToggleNoClipMode()
{
    m_NoClipMode = !m_NoClipMode;
}

void DebugManager::ToggleEnemyPathDisplay()
{
    m_ShowEnemyPath = !m_ShowEnemyPath;
}

void DebugManager::ToggleEnemyVisionDisplay()
{
    m_ShowEnemyVision = !m_ShowEnemyVision;
}

void DebugManager::ToggleInvincibleMode()
{
    m_InvincibleMode = !m_InvincibleMode;
}

void DebugManager::ToggleEnemyWhiskerDisplay()
{
    m_ShowEnemyWhisker = !m_ShowEnemyWhisker;
}

void DebugManager::ToggleLampLightsDisplay()
{
    m_ShowLampLights = !m_ShowLampLights;
}