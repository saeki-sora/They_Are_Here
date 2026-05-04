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

// コンストラクタ
Game::Game()
{
	m_Input = std::make_unique<Input>(); //入力�E琁E�E��E�作�E
	m_MainCamera = std::make_unique<Camera>(); //カメラを作�E
}

// チE�E��E�トラクタ
Game::~Game()
{

}

// インスタンスを取征E
Game& Game::GetInstance()
{
	static Game instance;
	return instance;
}

// 初期匁E
void Game::Init()
{
	Renderer::Init();// 描画初期匁E
	m_MainCamera->Init();// カメラ初期匁E

	SceneManager::GetInstance().Init();
	SceneManager::GetInstance().RegisterScene<TitleScene>();
	SceneManager::GetInstance().RegisterScene<DifficultySelectScene>(); // 難易度選択シーン
	SceneManager::GetInstance().RegisterScene<Stage1Scene>();
	SceneManager::GetInstance().RegisterScene<GameClearScene>();
	SceneManager::GetInstance().RegisterScene<GameOverScene>();

	DebugManager::Create();// チE�E��E�チE�E��E�マネージャーの作�E

	SimpleBoxCollider::InitDebugDraw(Renderer::GetDevice(), Renderer::GetDeviceContext()); // コライダーの初期匁E
	EffectManager::GetInstance().Init();//エフェクト�E初期匁E
	ConfigManager::GetInstance().Load("json/enemy_param.json");//設定ファイル読み込み

	SoundManager::GetInstance().Init();
	SoundManager::GetInstance().LoadSound("BGM_Title", L"assets/sound/amenisuteraretaningyou.wav");
	SoundManager::GetInstance().LoadSound("SE_KeyPickup", L"assets/sound/SE_KeyPickup.wav");

	SceneManager::GetInstance().ChangeScene<TitleScene>(std::make_unique<FadeTransition>(3.0f));// 3秒�Eフェードで遷移

}

// 更新
void Game::Update(float deltaTime)
{
	m_Input->Update();

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	ImGUI_Manager::DrawPanels();
	DebugManager::GetInstance().Update(deltaTime);// チE�E��E�チE�E��E�マネージャー更新
	EffectManager::GetInstance().Update(deltaTime);//エフェクト更新
	SoundManager::GetInstance().Update();//サウンド�Eネ�Eジャー更新

	SceneManager::GetInstance().Update(deltaTime);

	m_MainCamera->Update(deltaTime);
}

// 描画
void Game::Draw()
{
	// 描画前�E琁E
	// シャドウパス
	Renderer::BeginShadowPass();
	SceneManager::GetInstance().DrawShadow();
	Renderer::EndShadowPass();

	// 描画前の処理
	Renderer::Begin();

	// ===== 3D描画パス =====
	SceneManager::GetInstance().Draw();

	// ===== 2D描画パス =====
	SceneManager::GetInstance().DrawUI();

	EffectManager::GetInstance().Draw();//エフェクト描画

	SceneManager::GetInstance().DrawOverlayUI();//エフェクトの上に描画するUI（ミニマップなど）

	SceneManager::GetInstance().DrawTransition();//トランジション描画

	// 描画後�E琁E
	Renderer::End();
}

// 終亁E�E�E琁E
void Game::Uninit()
{
	// 描画終亁E�E�E琁E
	ImGUI_Manager::Uninit();

	Renderer::Uninit();

	SceneManager::GetInstance().Uninit();
	SoundManager::GetInstance().Uninit();//サウンド�Eネ�Eジャー終亁E�E�E琁E

	EffectManager::GetInstance().Uninit();

	DebugManager::Destroy();

	// カメラ終亁E�E�E琁E
	m_MainCamera->Uninit();

}
