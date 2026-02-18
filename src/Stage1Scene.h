#pragma once
#include "SceneBase.h"
#include "Object.h"
#include "MakeMap.h"
#include "MiniMap.h"
#include"VisualObject.h"

class Stage1Scene : public SceneBase
{
private:

    //イントロ用の状慁E
    enum class IntroPhase
    {
        GoalView,       // ゴールを少し離れた位置から眺める
        MoveToBirdEye,  // ゴール付迁EↁEマップ中央上空へ移勁E
        MoveToStart,    // マップ中央上空 ↁEスタート地点へ移勁E
        Finished        // イントロ終亁E��通常操作に移行！E
    };

    MakeMap m_MakeMap; // マップ生成クラス
    MiniMap m_MiniMap;// ミニマッチE

    float m_IntroTimer = 0.0f;
    DirectX::SimpleMath::Vector3 m_PlayerStartPos{}; // プレイヤーの開始位置�E�本来のスタート！E
    DirectX::SimpleMath::Vector3 m_GoalPos{};        // ゴールの位置
    DirectX::SimpleMath::Vector3 m_MapCenterPos{};   // スタートとゴールの中点
    DirectX::SimpleMath::Vector3 m_GoalViewPos{};    // ゴールを眺めるとき�E位置
    DirectX::SimpleMath::Vector3 m_BirdEyePos{};     // マップ中央の上空位置
    DirectX::SimpleMath::Vector3 m_StartForwardTarget{}; // スタート地点から少し前方の地面
    DirectX::SimpleMath::Vector3 m_BirdEyeLookTarget{};  // 上空から見るとき�EターゲチE��

    IntroPhase m_IntroPhase = IntroPhase::GoalView;  // 現在のイントロ状慁E

    // カメラ演�Eの設定（ハードコーチE��ング回避のため定数化！E
    static constexpr float GOAL_VIEW_DURATION = 2.0f;   // ゴールを眺める時間(私E
    static constexpr float CAMERA_MOVE_DURATION = 3.0f;   // 吁E��間�E移動時閁E私E
    static constexpr float BIRD_EYE_HEIGHT = 500.0f; // 俯瞰視点の高さ

    //「何�Eス刁E��すか�E�高さをどれだけ取るか」をブロチE��サイズ基準で決める
    static constexpr float GOAL_VIEW_DISTANCE_BLOCKS = 3.0f;   // ゴールから何�Eスぶん離れるぁE
    static constexpr float GOAL_VIEW_HEIGHT_BLOCKS = 1.8f;   // 壁より少し高い高さ(ブロチE��何個�EめE

    //スタートからどれくらい前方を見るか／俯瞰時にどれだけズラすか
    static constexpr float START_LOOK_AHEAD_BLOCKS = 2.0f; // スタートかめEマス刁E�Eを見る
    static constexpr float BIRD_EYE_LOOK_AHEAD_BLOCKS = 1.0f; // マップ中央から1マス刁E�Eを見る


    std::unique_ptr<VisualObject> m_UI_Capsule;//カプセルUI
    std::unique_ptr<VisualObject> m_BuckBord;//カプセルの背景UI

    std::unique_ptr<VisualObject> m_StaminaGauge;   // スタミナゲージ本佁E
    std::unique_ptr<VisualObject> m_StaminaGaugeBG; // スタミナゲージ背景


    //イントロ更新用の関数
    void UpdateIntro(float deltaTime);

public:
    Stage1Scene() = default;
    ~Stage1Scene() = default;

    void OnInit() override;
    void OnLoad() override;
    void OnUpdate(float deltaTime) override;
    void OnDrawUI() override;
    void OnUnload() override;
};
