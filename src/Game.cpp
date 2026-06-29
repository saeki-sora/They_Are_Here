#include "pch.h"
#include "Game.h"
#include "Renderer.h"
#include"SimpleBoxCollider.h"
#include "EffectManager.h"
#include "FadeEffect.h"
#include "SceneManager.h"
#include "TitleScene.h"
#include "DifficultySelectScene.h"
#include "Stage1Scene.h"
#include"ConfigManager.h"
#include "GameClearScene.h"
#include"GameOverScene.h"
#include"SoundManager.h"
#include "DebugManager.h"
#include "ImGUI_Manager.h"
#include "PostProcessManager.h"

// コンストラクタ
Game::Game()
{
	m_Input = std::make_unique<Input>(); //入力管理クラスの作成
	m_MainCamera = std::make_unique<Camera>(); //カメラを作成
}

// デストラクタ
Game::~Game()
{

}

// インスタンスを取得
Game& Game::GetInstance()
{
	static Game instance;
	return instance;
}

// 初期化
void Game::Init()
{
	Renderer::Init();// 描画初期化
	PostProcessManager::GetInstance().Init();// ポストプロセス初期化
	m_MainCamera->Init();// カメラ初期化

	SceneManager::GetInstance().Init();
	SceneManager::GetInstance().RegisterScene<TitleScene>();
	SceneManager::GetInstance().RegisterScene<DifficultySelectScene>(); // 難易度選択シーン
	SceneManager::GetInstance().RegisterScene<Stage1Scene>();
	SceneManager::GetInstance().RegisterScene<GameClearScene>();
	SceneManager::GetInstance().RegisterScene<GameOverScene>();

	DebugManager::Create();// チェックマネージャーの作成

	SimpleBoxCollider::InitDebugDraw(Renderer::GetDevice(), Renderer::GetDeviceContext()); // コライダーの初期化
	EffectManager::GetInstance().Init();//エフェクトの初期化
	ConfigManager::GetInstance().Load("json/enemy_param.json");//設定ファイル読み込み

	SoundManager::GetInstance().Init();
	SoundManager::GetInstance().LoadSound(SoundTag::BGM_Title,    L"assets/sound/amenisuteraretaningyou.wav");
	SoundManager::GetInstance().LoadSound(SoundTag::SE_KeyPickup, L"assets/sound/SE_KeyPickup.wav");
	SoundManager::GetInstance().LoadSound(SoundTag::SE_Found,     L"assets/sound/Found.wav");
	SoundManager::GetInstance().LoadSound(SoundTag::BGM_Stage1,   L"assets/sound/shinigamitowaltz.wav");
	SoundManager::GetInstance().LoadSound(SoundTag::BGM_Chase,    L"assets/sound/norowaretayakata.wav");

	SceneManager::GetInstance().ChangeScene<TitleScene>(std::make_unique<FadeTransition>(3.0f));// 3秒フェードで遷移

}

// 更新
void Game::Update(float deltaTime)
{
	m_Input->Update();

	DebugManager::GetInstance().Update(deltaTime);// チェックマネージャー更新
	EffectManager::GetInstance().Update(deltaTime);//エフェクト更新
	SoundManager::GetInstance().Update(deltaTime);//サウンドマネージャー更新
	PostProcessManager::GetInstance().Update(deltaTime);//ポストプロセス更新

	SceneManager::GetInstance().Update(deltaTime);

	m_MainCamera->Update(deltaTime);
}

// 描画
void Game::Draw()
{
	// ImGui フレーム開始
	ImGUI_Manager::BeginFrame();

	// 描画前の処理
	// シャドウパス（カメラ位置を中心に影を生成する）
	Renderer::BeginShadowPass(m_MainCamera->GetPosition());
	SceneManager::GetInstance().DrawShadow();
	Renderer::EndShadowPass();

	// 描画前の処理
	Renderer::Begin();

	// ===== 3D描画パス =====
	SceneManager::GetInstance().Draw();

	// ポストプロセス実行（HDRシーン → ブルーム/SSAO/トーンマップ等 → バックバッファ）
	PostProcessManager::GetInstance().Execute();

	// ===== 2D描画パス =====
	SceneManager::GetInstance().DrawUI();

	EffectManager::GetInstance().Draw();//エフェクト描画

	SceneManager::GetInstance().DrawOverlayUI();//エフェクトの上に描画するUI（ミニマップなど）

	SceneManager::GetInstance().DrawTransition();//トランジション描画

	// デバッグUI
	ImGUI_Manager::DrawPanels();

	// 描画後の処理
	Renderer::End();
}

// 終了処理
void Game::Uninit()
{
	// 描画終了処理
	ImGUI_Manager::Uninit();

	PostProcessManager::GetInstance().Uninit();//ポストプロセス終了処理
	Renderer::Uninit();

	SceneManager::GetInstance().Uninit();
	SoundManager::GetInstance().Uninit();//サウンドマネージャー終了処理

	EffectManager::GetInstance().Uninit();

	DebugManager::Destroy();

	// カメラ終了処理
	m_MainCamera->Uninit();

}
