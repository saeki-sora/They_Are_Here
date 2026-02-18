#include "pch.h"
#include "Stage1Scene.h"
#include "Game.h"
#include"VisualObject.h"
#include"Pole.h"
#include"Block.h"
#include "Player.h"
#include "SkyDome.h"
#include"EffectManager.h"
#include "FadeEffect.h"
#include "SceneManager.h"
#include "FadeTransition.h"
#include"Enemy.h"

using namespace DirectX::SimpleMath;


// ゴールやマップ中心を向くためのヘルパー
namespace
{
    void SetPlayerLookAt(Player& player,
        const Vector3& eyePos,
        const Vector3& targetPos,
        float rotateLerp = 0.15f) //補間率
    {
        Vector3 toTarget = targetPos - eyePos;
        float desiredYaw = std::atan2(toTarget.x, toTarget.z);
        float distanceXZ = std::sqrt(toTarget.x * toTarget.x + toTarget.z * toTarget.z);

        const float EPS = 0.01f;
        if (distanceXZ < EPS) distanceXZ = EPS;

        float desiredPitch = std::atan2(toTarget.y, distanceXZ);

        //ピッチを安全な範囲にクランプ
        const float maxPitch = DirectX::XM_PIDIV2 - 0.1f;
        desiredPitch = std::clamp(desiredPitch, -maxPitch, maxPitch);

        //ここから「現在の向き」→「目標の向き」へ滑らかに寄せる処理
        float currentYaw = player.GetYawRotation();
        float currentPitch = player.GetPitch();

        // --- Yaw の差を -PI〜PI に正規化して「短い方の回転」にする ---
        float deltaYaw = desiredYaw - currentYaw;
        while (deltaYaw > DirectX::XM_PI)  deltaYaw -= DirectX::XM_2PI;
        while (deltaYaw < -DirectX::XM_PI) deltaYaw += DirectX::XM_2PI;

        float newYaw = currentYaw + deltaYaw * rotateLerp;
        float newPitch = currentPitch + (desiredPitch - currentPitch) * rotateLerp;

        player.SetYawRotation(newYaw);
        player.SetPitch(newPitch);
    }

    // ゴール付近 → 上空 → マップ中央上空 という経路を作る
    Vector3 LerpUpThenAcross(const Vector3& from, const Vector3& to, float upHeight, float t)
    {
        t = std::clamp(t, 0.0f, 1.0f);

        Vector3 fromHigh = from;
        fromHigh.y = upHeight;

        Vector3 toHigh = to;
        toHigh.y = upHeight;

        if (t < 0.5f)
        {
            float tt = t * 2.0f;
            return Vector3::Lerp(from, fromHigh, tt);
        }
        else
        {
            float tt = (t - 0.5f) * 2.0f;
            return Vector3::Lerp(fromHigh, toHigh, tt);
        }
    }

    // マップ中央上空 → スタート上空 → スタート という経路を作る
    Vector3 LerpAcrossThenDown(const Vector3& from, const Vector3& to, float upHeight, float t)
    {
        t = std::clamp(t, 0.0f, 1.0f);

        Vector3 fromHigh = from;
        fromHigh.y = upHeight;

        Vector3 toHigh = to;
        toHigh.y = upHeight;

        if (t < 0.5f)
        {
            float tt = t * 2.0f;
            return Vector3::Lerp(fromHigh, toHigh, tt);
        }
        else
        {
            float tt = (t - 0.5f) * 2.0f;
            return Vector3::Lerp(toHigh, to, tt);
        }
    }
}




// バッググラウンド処理
void Stage1Scene::OnLoad()
{
    m_MakeMap.CreatePlan();
}


// 初期化
void Stage1Scene::OnInit()
{
	// カメラモードをプレイヤー追従に設定
    Game::GetInstance().GetMainCamera().SetMode(Camera::Mode::FollowPlayer);

    // スカイドームの追加
    auto skyDomePtr = AddObject<SkyDome>();
    if (auto sky = skyDomePtr.lock())
    {
        sky->SetScale(1000.0f, 1000.0f, 1000.0f);
        sky->SetTexture("assets/texture/sky.dds");
    }

    // ミニマップの追加
    auto weakMiniMap = AddObject<MiniMap>();
    if (auto miniMap = weakMiniMap.lock())
    {
        miniMap->SetMap(&m_MakeMap);
    }

    //透明化カプセルのUI
    m_UI_Capsule = std::make_unique<VisualObject>();
    m_UI_Capsule->Init();
    m_UI_Capsule->SetTexture("assets/texture/InvisibleItemCapsule.png");
    m_UI_Capsule->SetScale(100.0f, 100.0f, 1.0f);

	//カプセルUIの背景
    m_BuckBord = std::make_unique<VisualObject>();
    m_BuckBord->Init();
    m_BuckBord->SetTexture("assets/texture/BuckBord.png");
    m_BuckBord->SetScale(150.0f, 150.0f, 1.0f);

    // スタミナゲージUI
    m_StaminaGauge = std::make_unique<VisualObject>();
    m_StaminaGauge->Init();
    m_StaminaGauge->SetTexture("assets/texture/StaminaGauge.png");

    m_StaminaGaugeBG = std::make_unique<VisualObject>();
    m_StaminaGaugeBG->Init();
    m_StaminaGaugeBG->SetTexture("assets/texture/BuckBord.png");


    // マップオブジェクトの生成
    m_MakeMap.SpawnObjects(this);

    // プレイヤーとゴールを取得
    auto player = FindObject<Player>().lock();
    auto pole = FindObject<Pole>().lock();

    if (player && pole)
    {
        //元々のスタート位置を保存（イントロ後に戻す）
        m_PlayerStartPos = player->GetPosition();

        //ゴール位置を保存
        m_GoalPos = pole->GetPosition();

        //スタートとゴールの中点を「マップ中央」とみなす
        m_MapCenterPos = (m_PlayerStartPos + m_GoalPos) * 0.5f;

        const float blockSize = MAP::Config::BLOCK_SIZE;
        const float goalViewDistance = blockSize * GOAL_VIEW_DISTANCE_BLOCKS;   // 3マスぶん
        const float goalViewHeight = blockSize * GOAL_VIEW_HEIGHT_BLOCKS;     // 壁より少し上

        // ゴール → マップ中心方向のベクトル（水平）
        Vector3 toCenter = m_MapCenterPos - m_GoalPos;
        toCenter.y = 0.0f;
        if (toCenter.LengthSquared() < 1e-4f)
        {
            toCenter = Vector3::UnitZ;
        }
        toCenter.Normalize();

        //ゴールを見る位置（ゴールから少し離れた場所）
        m_GoalViewPos = m_GoalPos + toCenter * goalViewDistance;
        m_GoalViewPos.y = goalViewHeight;

        //「スタート → ゴール」方向を正面方向とみなす
        Vector3 forwardDir = m_GoalPos - m_PlayerStartPos;
        forwardDir.y = 0.0f;
        if (forwardDir.LengthSquared() < 1e-4f)
        {
            forwardDir = Vector3::UnitZ;
        }
        forwardDir.Normalize();

        //スタート地点から少し前方の地面（最終的に見ていてほしい場所）
        float startLookAhead = blockSize * START_LOOK_AHEAD_BLOCKS;
        m_StartForwardTarget = m_PlayerStartPos + forwardDir * startLookAhead;
        m_StartForwardTarget.y = m_PlayerStartPos.y;

        //上空から見るときも「マップ中心から少し前方」を見るようにする
        float birdLookAhead = blockSize * BIRD_EYE_LOOK_AHEAD_BLOCKS;
        m_BirdEyeLookTarget = m_MapCenterPos + forwardDir * birdLookAhead;
        m_BirdEyeLookTarget.y = m_MapCenterPos.y;

        //マップ中央の上空位置
        m_BirdEyePos = m_MapCenterPos;
        m_BirdEyePos.y = BIRD_EYE_HEIGHT;

        //まずは「ゴールを少し離れた位置から見る」場所にプレイヤーを移動
        player->SetPosition(m_GoalViewPos);
        SetPlayerLookAt(*player, m_GoalViewPos, m_GoalPos); // ゴール方向を見る

        //イントロ中は移動と視点操作を禁止
        player->SetCanMove(false);
        player->SetMouseCaptured(false);
    }

    // イントロの状態初期化
    m_IntroTimer = 0.0f;
    m_IntroPhase = IntroPhase::GoalView;
}




// 更新
void Stage1Scene::OnUpdate(float deltaTime)
{
    //まずイントロ演出を進める
    if (m_IntroPhase != IntroPhase::Finished)
    {
        UpdateIntro(deltaTime);
    }

	Enemy::ResetPathCalculationCount();// 今フレームのパス計算数をリセットする
}



void Stage1Scene::UpdateIntro(float deltaTime)
{
    auto player = FindObject<Player>().lock();
    if (!player)
    {
        m_IntroPhase = IntroPhase::Finished;
        return;
    }

    m_IntroTimer += deltaTime;

    switch (m_IntroPhase)
    {
    case IntroPhase::GoalView:
    {
        // ゴールをしばらく眺める
        player->SetPosition(m_GoalViewPos);

        //第3引数に m_GoalPos（見るターゲット）、第4引数に 1.0f（即向く）
        SetPlayerLookAt(*player, m_GoalViewPos, m_GoalPos, 1.0f);

        if (m_IntroTimer >= GOAL_VIEW_DURATION)
        {
            m_IntroPhase = IntroPhase::MoveToBirdEye;
            m_IntroTimer = 0.0f;
        }
    }
    break;


    case IntroPhase::MoveToBirdEye:
    {
        // ゴール付近 → マップ中央上空へ
        float t = std::clamp(m_IntroTimer / CAMERA_MOVE_DURATION, 0.0f, 1.0f);

        //「上に上がってから水平移動」
        Vector3 pos = LerpUpThenAcross(m_GoalViewPos, m_BirdEyePos, BIRD_EYE_HEIGHT, t);
        player->SetPosition(pos);

        //ここではずっとゴールを見続ける（ターゲットを動かさない）
        SetPlayerLookAt(*player, pos, m_GoalPos);

        if (t >= 1.0f)
        {
            m_IntroPhase = IntroPhase::MoveToStart;
            m_IntroTimer = 0.0f;
        }
    }
    break;


    case IntroPhase::MoveToStart:
    {
        // マップ中央上空 → 本来のスタート地点へ
        float t = std::clamp(m_IntroTimer / CAMERA_MOVE_DURATION, 0.0f, 1.0f);

        //上空で水平移動 → 最後にスタート位置まで降りてくる
        Vector3 pos = LerpAcrossThenDown(m_BirdEyePos, m_PlayerStartPos, BIRD_EYE_HEIGHT, t);
        player->SetPosition(pos);

        //見る先は「ゴール」→「スタート前方ターゲット」へ徐々に遷移
        Vector3 lookTarget = Vector3::Lerp(m_GoalPos, m_StartForwardTarget, t);
        SetPlayerLookAt(*player, pos, lookTarget);

        if (t >= 1.0f)
        {
            // ぴったりスタート位置に合わせる & 最終的な向きで固定
            player->SetPosition(m_PlayerStartPos);
            SetPlayerLookAt(*player, m_PlayerStartPos, m_StartForwardTarget);

            //ここで操作解禁
            player->SetCanMove(true);
            player->SetMouseCaptured(true);

            m_IntroPhase = IntroPhase::Finished;
            m_IntroTimer = 0.0f;
        }
    }
    break;


    case IntroPhase::Finished:
        // 何もしない（通常プレイ）
        break;
    }
}




void Stage1Scene::OnDrawUI()
{
    // プレイヤーを取得
    if (auto player = FindObject<Player>().lock())
    {
        int currentStock = player->GetInvisibleStock(); // 現在のストック
        int maxStock = player->GetMaxInvisibleStock();  // プレイヤーから最大値を取得

        // 画面上の表示位置
        float startX = -500.0f;
        float startY = -250.0f;
        float gapX = 60.0f;

		// 背景の位置とサイズを計算
        float totalWidth = (maxStock - 1) * gapX;

        float bgCenterX = startX + (totalWidth * 0.5f);
        float bgScaleX = totalWidth + 120.0f; // 余白

        // 背景の位置とサイズを適用
        m_BuckBord->SetPosition(bgCenterX, startY, 0.0f);
        m_BuckBord->SetScale(bgScaleX, 150.0f, 1.0f);

        // 背景を描画
        m_BuckBord->Draw();

        // ストックの数だけ位置をずらして描画
        for (int i = 0; i < currentStock; ++i)
        {
            // 位置セット
            m_UI_Capsule->SetPosition(startX + (i * gapX), startY, 0.0f);

            // 描画実行
            m_UI_Capsule->Draw();
        }

        // スタミナゲージ表示（画面右下）
        float ratio = player->GetStamina() / player->GetMaxStamina();
        float maxWidth  = 200.0f;
        float barHeight = 20.0f;
        float barX      = 400.0f;   // 画面右側
        float barY      = -300.0f;  // 画面下側

        // 背景（黒帯、フルサイズ）
        m_StaminaGaugeBG->SetPosition(barX, barY, 0.0f);
        m_StaminaGaugeBG->SetScale(maxWidth + 10.0f, barHeight + 10.0f, 1.0f);
        m_StaminaGaugeBG->Draw();

        // ゲージ本体（スタミナ % でスケール）
        float currentWidth = maxWidth * ratio;
        float offsetX = (maxWidth - currentWidth) * -0.5f; // 左端基準で縮小
        m_StaminaGauge->SetPosition(barX + offsetX, barY, 0.0f);
        m_StaminaGauge->SetScale(currentWidth, barHeight, 1.0f);
        m_StaminaGauge->Draw();
    }
}



// 終了処理
void Stage1Scene::OnUnload()
{
    SceneBase::OnUnload();
}



