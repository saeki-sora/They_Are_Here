#include "Game.h"
#include "Renderer.h"
#include"SimpleBoxCollider.h"
#include "EffectManager.h"
#include "FadeEffect.h"
#include "SceneManager.h"
#include "TitleScene.h"
#include "Stage1Scene.h"
#include "ResultScene.h"

// コンストラクタ
Game::Game()
{
	m_Input = std::make_unique<Input>(); //入力処理を作成
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
	m_MainCamera->Init();// カメラ初期化

	SceneManager::GetInstance().Init();
	SceneManager::GetInstance().RegisterScene<TitleScene>();
	SceneManager::GetInstance().RegisterScene<Stage1Scene>();
	SceneManager::GetInstance().RegisterScene<ResultScene>();
	SceneManager::GetInstance().ChangeScene<TitleScene>();

	SimpleBoxCollider::InitDebugDraw(Renderer::GetDevice(), Renderer::GetDeviceContext()); // コライダーの初期化
	EffectManager::GetInstance().Init();//エフェクトの初期化

}

// 更新
void Game::Update(float deltaTime)
{
	m_Input->Update();// 入力処理更新
	EffectManager::GetInstance().Update();//エフェクト更新

	SceneManager::GetInstance().Update(deltaTime);

	m_MainCamera->Update();
}

// 描画
void Game::Draw()
{
	// 描画前処理
	Renderer::Begin();

	// ===== 3D描画パス =====
	SceneManager::GetInstance().Draw();

	// ===== 2D描画パス =====
	SceneManager::GetInstance().DrawUI();

	EffectManager::GetInstance().Draw();//エフェクト描画

	SceneManager::GetInstance().DrawTransition();//トランジション描画

	// 描画後処理
	Renderer::End();
}

// 終了処理
void Game::Uninit()
{
	SceneManager::GetInstance().Uninit();

	//オブジェクトを全て削除
	DeleteAllObject();

	EffectManager::GetInstance().Uninit();

	// カメラ終了処理
	m_MainCamera->Uninit();

	// 描画終了処理
	Renderer::Uninit();
}


// オブジェクトを削除する
void Game::DeleteObject(std::weak_ptr<Object> weak_pt)
{
	if (auto shared_pt = weak_pt.lock())
	{
		auto it = std::find(m_Objects.begin(), m_Objects.end(), shared_pt);

		if (it != m_Objects.end()) 
		{
			(*it)->Uninit();
			m_Objects.erase(it);
		}
	}
}


// オブジェクトをすべて削除する
void Game::DeleteAllObject()
{
	// オブジェクト終了処理
	for (auto& o : m_Objects)
	{
		o->Uninit();
	}

	m_Objects.clear(); //全て削除
	m_Objects.shrink_to_fit();
}


// 削除されたオブジェクトのクリーンアップ
void Game::CleanupDeadObjects()
{
	size_t originalSize = m_Objects.size();

	// shared_ptrの参照カウントが1（このコンテナのみ）のオブジェクトを削除
	m_Objects.erase(
		std::remove_if(m_Objects.begin(), m_Objects.end(),
			[](const std::shared_ptr<Object>& obj) {
				return obj.use_count() <= 1; // このコンテナ以外に参照がない
			}),
		m_Objects.end()
	);

	// オブジェクトが実際に削除された場合はキャッシュを無効化
	if (m_Objects.size() != originalSize) {
		InvalidateCache();
	}
}

