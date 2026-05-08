#pragma once
#include "SceneBase.h"
#include "Object.h"
#include "MiniMap.h"
#include "VisualObject.h"
#include "Renderer.h"
#include "PauseMenu.h"

class Stage1Scene : public SceneBase
{
private:

	//イントロ用の状態管理
    enum class IntroPhase
    {
        GoalView,       // ゴールを少し離れた位置から眺める
        MoveToBirdEye,  // ゴール付近からマップ中央上空へ移動
        MoveToStart,    // マップ中央上空 からスタート地点へ移動
        Finished        // イントロ終了、通常操作に移行！
    };

    std::shared_ptr<MiniMap> m_MiniMap; // ミニマップ（エフェクトの上に描画するため個別管理）

    float m_IntroTimer = 0.0f;
    DirectX::SimpleMath::Vector3 m_PlayerStartPos{}; // プレイヤーの開始位置（本来のスタート位置）
    DirectX::SimpleMath::Vector3 m_GoalPos{};        // ゴールの位置
    DirectX::SimpleMath::Vector3 m_MapCenterPos{};   // スタートとゴールの中点
    DirectX::SimpleMath::Vector3 m_GoalViewPos{};    // ゴールを眺めるときの位置
    DirectX::SimpleMath::Vector3 m_BirdEyePos{};     // マップ中央の上空位置
    DirectX::SimpleMath::Vector3 m_StartForwardTarget{}; // スタート地点から少し前方の地面
    DirectX::SimpleMath::Vector3 m_BirdEyeLookTarget{};  // 上空から見るときのターゲット

    IntroPhase m_IntroPhase = IntroPhase::GoalView;  // 現在のイントロ状況

    // ライトちらつき用
    float m_LightFlickerTimer = 0.0f;
    LIGHT m_BaseLight{};  // 基準ライト設定を保存

    // インゲームのBGM状態管理
    float m_BGMVolume        = 1.0f;  // 現在のBGM音量
    bool  m_BGMStarted       = false; // フェードイン完了後にBGMを開始したかどうか
    bool  m_IsEnemyChasing   = false; // 前フレームにいずれかの敵が追跡中だったか
    float m_BGMResumeTimer   = 0.0f;  // 追跡解除後のBGM再開待機タイマー

    // カメラ演出の設定
    static constexpr float GOAL_VIEW_DURATION = 2.0f;   // ゴールを眺める時間(秒)
	static constexpr float CAMERA_MOVE_DURATION = 3.0f;   // カメラ移動の時間(秒)
    static constexpr float BIRD_EYE_HEIGHT = 500.0f; // 俯瞰視点の高さ

	// ゴールを眺めるときの位置は、ゴールから少し離れて、壁より少し高い位置にする
    static constexpr float GOAL_VIEW_DISTANCE_BLOCKS = 3.0f;   // ゴールから何ブロックぶん離れるか
    static constexpr float GOAL_VIEW_HEIGHT_BLOCKS = 1.8f;   // 壁より少し高い高さ(ブロック何個分)

    //スタートからどれくらい前方を見るか／俯瞰時にどれだけズラすか
    static constexpr float START_LOOK_AHEAD_BLOCKS = 2.0f; // スタートから2マス前を見る
    static constexpr float BIRD_EYE_LOOK_AHEAD_BLOCKS = 1.0f; // マップ中央から1マス前を見る

    // 敵距離によるBGM音量制御
    static constexpr float BGM_NEAR_DIST  = 180.0f; // この距離以内で最小音量
    static constexpr float BGM_FAR_DIST   = 500.0f; // この距離以上で最大音量
    static constexpr float BGM_MIN_VOLUME    = 0.15f; // 敵が近いときの最小音量
    static constexpr float BGM_LERP_SPEED   = 2.0f;  // 音量補間速度（/秒）
    static constexpr float BGM_RESUME_DELAY = 1.5f;  // 追跡解除後にBGMを再開するまでの秒数


    std::unique_ptr<VisualObject> m_UI_Capsule;//カプセルUI
    std::unique_ptr<VisualObject> m_BuckBord;//カプセルの背景UI

    std::unique_ptr<VisualObject> m_StaminaGauge;   // スタミナゲージ本体
    std::unique_ptr<VisualObject> m_StaminaGaugeBG; // スタミナゲージ背景

	std::unique_ptr<VisualObject> m_OptionGuide; // 操作説明UI

    std::unique_ptr<PauseMenu> m_OptionMenu;  // ポーズメニュー


    //イントロ更新用の関数
    void UpdateIntro(float deltaTime);

public:
    Stage1Scene() = default;
    ~Stage1Scene() = default;

    void OnInit() override;
    void OnLoad() override;
    void OnUpdate(float deltaTime) override;
    void OnDrawUI() override;
    void OnDrawOverlayUI() override; // エフェクトの上に描画（ミニマップ）
    void OnUnload() override;
};
