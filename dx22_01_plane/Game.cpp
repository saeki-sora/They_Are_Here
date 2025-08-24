#include "Game.h"
#include "Renderer.h"
#include"SimpleBoxCollider.h"
#include "Skybox.h"

// コンストラクタ
Game::Game()
{
	m_Input = std::make_unique<Input>(); //入力処理を作成
	m_MainCamera = std::make_unique<Camera>(); //カメラを作成
}

// デストラクタ
Game::~Game()
{
	m_Scene.reset();
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
	// 描画初期化
	Renderer::Init();

	// カメラ初期化
	m_MainCamera->Init();

	m_Scene = std::make_unique<TitleScene>();

	SimpleBoxCollider::InitDebugDraw(Renderer::GetDevice(), Renderer::GetDeviceContext()); // コライダーの初期化

}

// 更新
void Game::Update()
{
	m_Scene->Update();// シーン更新

	m_Input->Update();// 入力処理更新

	for (auto& o : m_Objects)// オブジェクト更新
	{
		o->Update();
	}

	m_MainCamera->Update(); // カメラ更新
}

// 描画
void Game::Draw()
{

	// 描画前処理
	Renderer::Begin();

	// ===== 3D描画パス =====
	m_MainCamera->SetCamera(0);

	// オブジェクト描画
	for (auto& obj : m_Objects)
	{
		if (obj->Is3D()) // ← 描画タイプ判定
		{
			obj->Draw();
		}
	}

	// ===== 2D描画パス =====
	m_MainCamera->SetCamera(1); // 2Dカメラに設定

	for (auto& obj : m_Objects)
	{
		if (!obj->Is3D())
		{
			obj->Draw();
		}
	}

	// 描画後処理
	Renderer::End();
}

// 終了処理
void Game::Uninit()
{

	// シーン終了処理
	m_Scene.reset();

	//オブジェクトを全て削除
	DeleteAllObject();

	// カメラ終了処理
	m_MainCamera->Uninit();

	// 描画終了処理
	Renderer::Uninit();
}


// シーンを切り替える
void Game::ChangeScene(SceneName sName)
{

	// 読み込み済みのシーンがあれば削除
	if (m_Scene != nullptr) 
	{
		m_Scene.reset();
	}

	switch (sName)
	{
	case SceneName::TITLE:
		m_Scene = std::make_unique<TitleScene>();
		break;
	case SceneName::STAGE1:
		m_Scene = std::make_unique<Stage1Scene>();
		break;
	case SceneName::RESULT:
		m_Scene = std::make_unique<ResultScene>();
		break;
	}
}

// オブジェクトを削除する
void Game::DeleteObject(std::weak_ptr<Object> weak_pt)
{
	if (auto shared_pt = weak_pt.lock()) {
		auto it = std::find(m_Objects.begin(), m_Objects.end(), shared_pt);

		if (it != m_Objects.end()) {
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