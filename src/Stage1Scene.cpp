#include "pch.h"
#include "Stage1Scene.h"
#include "Game.h"
#include "VisualObject.h"
#include "Pole.h"
#include "Block.h"
#include "Player.h"
#include "SkyDome.h"
#include "EffectManager.h"
#include "FadeEffect.h"
#include "SceneManager.h"
#include "FadeTransition.h"
#include "Enemy.h"
#include "GoalKey.h"
#include "DifficultyManager.h"
#include "Application.h"
#include "TitleScene.h"
#include "SoundManager.h"
#include "ResultData.h"

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




// バックグラウンド処理
void Stage1Scene::OnLoad()
{

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

	// ミニマップ（エフェクトの上に描画したいため、個別管理）
	MakeMap& selectedMap = DifficultyManager::GetInstance().GetSelectedMap();
	m_MiniMap = std::make_shared<MiniMap>();
	m_MiniMap->Init();
	m_MiniMap->SetMap(&selectedMap);

	//透明化カプセルのUI
	m_UI_Capsule = std::make_unique<VisualObject>();
	m_UI_Capsule->Init();
	m_UI_Capsule->SetTexture("assets/texture/InvisibleItemCapsule.png");
	m_UI_Capsule->SetScale(70.0f, 70.0f, 1.0f);

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

	m_OptionGuide = std::make_unique<VisualObject>();
	m_OptionGuide->Init();
	m_OptionGuide->SetTexture("assets/texture/OptionGuide.png");

	m_OptionMenu = std::make_unique<PauseMenu>();
	m_OptionMenu->Init();

	m_KeyPickupNotice = std::make_unique<KeyPickupNotice>();
	m_KeyPickupNotice->Init();
	m_HadKeyLastFrame = false;

	// マップオブジェクトの生成
	selectedMap.SpawnObjects(this);

	// ミニマップに鍵とゴールの参照をセット
	if (m_MiniMap)
	{
		if (auto key = FindObject<GoalKey>().lock())
			m_MiniMap->SetGoalKey(key);
		if (auto pole = FindObject<Pole>().lock())
			m_MiniMap->SetGoalPole(pole);
	}

	// プレイヤーとゴールを取得
	auto player = FindObject<Player>().lock();
	auto pole = FindObject<Pole>().lock();

	// 難易度設定をプレイヤーに適用する
	if (player)
	{
		const DifficultyConfig& cfg = DifficultyManager::GetInstance().GetSelectedConfig();
		player->SetMaxInvisibleStock(cfg.maxInvisibleStock);
	}

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

	// ライトちらつき用にベースライトを保存
	m_BaseLight.Enable = TRUE;
	m_BaseLight.Direction = DirectX::SimpleMath::Vector4(0.5f, -1.0f, 0.8f, 0.0f);
	m_BaseLight.Direction.Normalize();
	m_BaseLight.Diffuse = DirectX::SimpleMath::Color(0.55f, 0.62f, 0.85f, 1.0f);
	m_BaseLight.Ambient = DirectX::SimpleMath::Color(0.55f, 0.36f, 0.59f, 1.0f);
	m_LightFlickerTimer = 0.0f;
}




// 更新
void Stage1Scene::OnUpdate(float deltaTime)
{
	// ポーズ処理
	auto& cam = Game::GetInstance().GetMainCamera();
	PauseAction pauseAction = m_OptionMenu->Update(deltaTime);

	switch (pauseAction)
	{
	case PauseAction::Open:

		SetGamePaused(true);
		cam.ReleaseMouseImmediate();
		cam.SetClickToRecapture(false); // ポーズ中はクリックで誤ってキャプチャされないよう無効化
		break;

	case PauseAction::Close:

		SetGamePaused(false);
		cam.SetClickToRecapture(true);
		if (m_IntroPhase == IntroPhase::Finished)
			cam.RecaptureMouseImmediate();
		break;

	case PauseAction::ReturnToTitle:

		SetGamePaused(false);
		SceneManager::GetInstance().ChangeScene<TitleScene>(
			std::make_unique<FadeTransition>(2.5f));
		return;

	default:
		break;
	}

	if (IsGamePaused()) return; // オブジェクト更新は SceneBase がスキップ

	// ライトのちらつき
	m_LightFlickerTimer += deltaTime;
	float flicker = 0.95f
		+ 0.01f * sinf(m_LightFlickerTimer * 0.7f)
		+ 0.01f * sinf(m_LightFlickerTimer * 1.3f);

	LIGHT flickerLight = m_BaseLight;
	flickerLight.Ambient.x *= flicker;
	flickerLight.Ambient.y *= flicker;
	flickerLight.Ambient.z *= flicker;
	Renderer::SetLight(flickerLight);

	//まずイントロ演出を進める
	if (m_IntroPhase != IntroPhase::Finished)
	{
		UpdateIntro(deltaTime);
	}

	// イントロ終了後、ポーズ中以外でプレイ時間を計測
	if (m_IntroPhase == IntroPhase::Finished && !IsGamePaused())
	{
		m_GameTimer += deltaTime;
	}
	ResultData::s_ElapsedTime = m_GameTimer;

	Enemy::ResetPathCalculationCount();// 今フレームのパス計算数をリセットする

	// カギ取得通知：取得した瞬間だけ再生する
	if (auto player = FindObject<Player>().lock())
	{
		bool hasKey = player->HasKey();
		if (hasKey && !m_HadKeyLastFrame)
		{
			m_KeyPickupNotice->Play();
		}
		m_HadKeyLastFrame = hasKey;
	}
	m_KeyPickupNotice->Update(deltaTime);

	// BGM管理：開始・追跡中のPause/Resume・距離ベース音量制御
	{
		// フェードイン完了後に一度だけBGMを開始
		if (!m_BGMStarted && !SceneManager::GetInstance().IsTransitioning())
		{
			SoundManager::GetInstance().PlayBGM(SoundTag::BGM_Stage1, true);
			m_BGMVolume = 1.0f;
			m_BGMStarted = true;
		}

		if (m_BGMStarted)
		{
			auto enemies = FindAllObjects<Enemy>();
			bool anyChasing = false;
			float minDist = FLT_MAX;

			if (auto player = FindObject<Player>().lock())
			{
				for (auto& weak : enemies)
				{
					if (auto e = weak.lock())
					{
						if (e->IsChasing()) anyChasing = true;
						float d = (e->GetPosition() - player->GetPosition()).Length();
						if (d < minDist) minDist = d;
					}
				}
			}

			if (anyChasing)
			{
				if (!m_IsEnemyChasing)
				{
					// Normal→Chase: 通常BGMをポーズしてチェイスBGMを開始
					SoundManager::GetInstance().PauseBGM(SoundTag::BGM_Stage1);
					SoundManager::GetInstance().PlayBGM(SoundTag::BGM_Chase, true);
					m_IsEnemyChasing = true;
				}

				// フェードアウト中に再発見 → チェイスBGMを即時フルボリュームに戻す
				if (m_BGMResumeTimer > 0.0f)
				{
					SoundManager::GetInstance().SetBGMVolume(SoundTag::BGM_Chase, 1.0f);
					m_BGMResumeTimer = 0.0f;
				}
			}
			else
			{
				if (m_IsEnemyChasing)
				{
					// 追跡解除後、BGM_RESUME_DELAY 秒かけてチェイスBGMをフェードアウト
					m_BGMResumeTimer += deltaTime;
					float t = std::min(m_BGMResumeTimer / BGM_RESUME_DELAY, 1.0f);
					if (t >= 1.0f)
					{
						SoundManager::GetInstance().StopBGM(SoundTag::BGM_Chase);
						SoundManager::GetInstance().ResumeBGM(SoundTag::BGM_Stage1);
						m_IsEnemyChasing = false;
						m_BGMResumeTimer = 0.0f;
					}
					else
					{
						SoundManager::GetInstance().SetBGMVolume(SoundTag::BGM_Chase, 1.0f - t);
					}
				}
				else
				{
					// 通常時：距離ベースの音量制御
					float t = std::clamp(
						(minDist - BGM_NEAR_DIST) / (BGM_FAR_DIST - BGM_NEAR_DIST),
						0.0f, 1.0f);
					float targetVolume = BGM_MIN_VOLUME + (1.0f - BGM_MIN_VOLUME) * t;
					m_BGMVolume += (targetVolume - m_BGMVolume) * std::min(BGM_LERP_SPEED * deltaTime, 1.0f);
					SoundManager::GetInstance().SetBGMVolume(SoundTag::BGM_Stage1, m_BGMVolume);
				}
			}
		}
	}

	// ミニマップの更新（個別管理のため手動呼び出し）
	if (m_MiniMap)
	{
		m_MiniMap->Update(deltaTime);
	}
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
		float startX = 500.0f;
		float startY = -220.0f;
		float gapX = 60.0f;// ストックアイコン同士の間隔

		// 背景の位置とサイズを計算
		float totalWidth = (maxStock - 1) * gapX;

		float bgCenterX = startX + (totalWidth * 0.5f);
		float bgScaleX = totalWidth + 120.0f; // 余白

		// 背景の位置とサイズを適用
		m_BuckBord->SetPosition(bgCenterX, startY, 0.0f);
		m_BuckBord->SetScale(bgScaleX, 100.0f, 1.0f);

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
		float maxWidth = 200.0f;
		float barHeight = 20.0f;
		float barX = 430.0f;   // 画面右側
		float barY = -330.0f;  // 画面下側

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

	const float tutW = 350.0f;
	const float tutH = 80.0f;

	const float tutX = -440.0f;
	const float tutY = 300.0f;

	m_OptionGuide->SetScale(tutW, tutH, 1.0f);
	m_OptionGuide->SetPosition(tutX, tutY, 0.0f);
	m_OptionGuide->Draw();
}



// エフェクトより上に描画するUI（カギ取得通知・ミニマップ・ポーズメニュー）
void Stage1Scene::OnDrawOverlayUI()
{
	// カギ取得通知（透明化エフェクトに隠れないよう、エフェクトより手前に描画する）
	m_KeyPickupNotice->Draw();

	if (m_MiniMap)
	{
		m_MiniMap->Draw();
	}

	// ポーズメニューは最前面に描画（非アクティブ時は何もしない）
	m_OptionMenu->Draw();
}


// 終了処理
void Stage1Scene::OnUnload()
{
	SoundManager::GetInstance().StopAllBGM();

	// ミニマップを個別管理しているため、手動で Uninit を呼ぶ
	if (m_MiniMap)
	{
		m_MiniMap->Uninit();
		m_MiniMap.reset();
	}

	if (m_OptionMenu)
	{
		m_OptionMenu->Uninit();
		m_OptionMenu.reset();
	}

	if (m_KeyPickupNotice)
	{
		m_KeyPickupNotice->Uninit();
		m_KeyPickupNotice.reset();
	}

	SceneBase::OnUnload();
}



