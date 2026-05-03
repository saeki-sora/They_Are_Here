#include "pch.h"
#include "SceneManager.h"
#include "Game.h"
#include "Renderer.h"
#include "FadeTransition.h"

namespace 
{
	// optionalの中身があればそれをキーにしてmapを検索、なければend()を返す
	template <class Map>
	auto find_or_end(Map& m, const std::optional<SceneId>& idopt) 
	{
		return idopt ? m.find(*idopt) : m.end();
	}
}

// ================================================================================
// SceneManager実装
// ================================================================================

SceneManager::~SceneManager()
{
	Uninit();
}

SceneManager& SceneManager::GetInstance()
{
	static SceneManager instance;
	return instance;
}



void SceneManager::Init()
{
	// 初期化処理
	m_IsTransitioning = false;
	m_IsLoading = false;
}



void SceneManager::Uninit()
{
	// 全シーンのアンロード
	for (auto& [id, info] : m_Scenes)
	{
		if (info.scene && info.state != SceneState::Unloaded) {
			info.scene->OnDeactivate();
			info.scene->OnUnload();
		}
	}
	m_Scenes.clear();
	m_SceneFactories.clear();
}



// シーン変更の内部実装
void SceneManager::ChangeSceneInternal(SceneId sceneId, std::unique_ptr<SceneTransition> transition)
{
	if (m_IsTransitioning) return;

	auto factoryIt = m_SceneFactories.find(sceneId);
	if (factoryIt == m_SceneFactories.end()) return;

	m_NextSceneId = sceneId;
	m_Transition = std::move(transition);
	m_IsTransitioning = true;

	if (!m_Transition)
	{
		// 現在のシーンを非アクティブ化
		auto currentIt = find_or_end(m_Scenes, m_CurrentSceneId);
		if (currentIt != m_Scenes.end() && currentIt->second.scene)
		{
			currentIt->second.scene->OnDeactivate();
			currentIt->second.state = SceneState::Ready;
		}

		// 次のシーンを準備
		auto& nextInfo = m_Scenes[*m_NextSceneId];
		if (!nextInfo.scene) 
		{
			nextInfo.scene = factoryIt->second();
			nextInfo.state = SceneState::Loading;
			nextInfo.scene->OnLoad();
			nextInfo.scene->OnInit();
			nextInfo.state = SceneState::Ready;
		}
		else
		{
			//PreloadでOnLoadだけ終わってるケース
			if (nextInfo.state == SceneState::Ready)
			{
				nextInfo.scene->OnInit();
			}
		}

		// アクティブ化
		nextInfo.scene->OnActivate();
		nextInfo.state = SceneState::Active;

		m_CurrentSceneId = m_NextSceneId;
		m_IsTransitioning = false;

		if (m_OnSceneChanged) m_OnSceneChanged();
	}
	else
	{
		m_Transition->Start();

		if (!m_CurrentSceneId)  //起動直後の最初の ChangeScene だけ true になる
		{
			if (auto fade = dynamic_cast<FadeTransition*>(m_Transition.get()))
			{
				//タイマーをロードフェーズに進める
				fade->SetPhase(SceneTransition::Phase::Loading);
			}
		}
	}
}




void SceneManager::PreloadSceneInternal(SceneId sceneId)
{
	auto factoryIt = m_SceneFactories.find(sceneId);
	if (factoryIt == m_SceneFactories.end()) return;


	auto& info = m_Scenes[sceneId];
	if (info.scene || info.state == SceneState::Loading)
	{
		return; // 既にロード済みまたはロード中
	}

	// 非同期ロード開始
	info.state = SceneState::Loading;
	info.loadStartTime = std::chrono::steady_clock::now();

	info.loadingTask = std::async(std::launch::async, [this, sceneId, &info, factoryIt]()
		{
			info.scene = factoryIt->second();
			info.scene->OnLoad();

			// メインスレッドで初期化が必要な場合はフラグを立てる
			info.state = SceneState::Ready;
		});
}




void SceneManager::UnloadSceneInternal(SceneId sceneId)
{
	auto it = m_Scenes.find(sceneId);
	if (it == m_Scenes.end()) return;

	// 現在のシーンはアンロードできない
	if (m_CurrentSceneId && sceneId == *m_CurrentSceneId) return; // ← 修正

	auto& info = it->second;
	if (info.scene && info.state != SceneState::Unloaded) {
		info.state = SceneState::Unloading;
		info.scene->OnUnload();
		info.scene.reset();
		info.state = SceneState::Unloaded;
	}
}



void SceneManager::Update(float deltaTime)
{
	ProcessLoadingQueue();

	if (m_IsTransitioning && m_Transition) {
		UpdateTransition(deltaTime);
	}

	auto currentIt = find_or_end(m_Scenes, m_CurrentSceneId);

	if (currentIt != m_Scenes.end() &&
		currentIt->second.scene &&
		currentIt->second.state == SceneState::Active)
	{
		currentIt->second.scene->Update(deltaTime);
	}
}



void SceneManager::Draw()
{
	Game::GetInstance().GetMainCamera().SetCamera(0); // 3Dカメラに設定

	auto currentIt = find_or_end(m_Scenes, m_CurrentSceneId);
	if (currentIt != m_Scenes.end() &&
		currentIt->second.scene &&
		currentIt->second.state == SceneState::Active)
	{
		currentIt->second.scene->Draw();
	}
}

void SceneManager::DrawUI()
{
	Game::GetInstance().GetMainCamera().SetCamera(1); // 2Dカメラに設定

	auto currentIt = find_or_end(m_Scenes, m_CurrentSceneId);
	if (currentIt != m_Scenes.end() &&
		currentIt->second.scene &&
		currentIt->second.state == SceneState::Active)
	{
		currentIt->second.scene->DrawUI();//UI描画
	}
}

void SceneManager::DrawOverlayUI()
{
	Game::GetInstance().GetMainCamera().SetCamera(1); // 2Dカメラに設定

	auto currentIt = find_or_end(m_Scenes, m_CurrentSceneId);
	if (currentIt != m_Scenes.end() &&
		currentIt->second.scene &&
		currentIt->second.state == SceneState::Active)
	{
		currentIt->second.scene->DrawOverlayUI();
	}
}


void SceneManager::DrawTransition()
{
	// シーン遷移中であれば、トランジションエフェクトを描画
	if (m_IsTransitioning && m_Transition) {
		m_Transition->Draw();
	}
}

void SceneManager::DrawShadow()
{
	Game::GetInstance().GetMainCamera().SetCamera(0); // 3Dカメラに設定

	auto currentIt = find_or_end(m_Scenes, m_CurrentSceneId);
	if (currentIt != m_Scenes.end() &&
		currentIt->second.scene &&
		currentIt->second.state == SceneState::Active)
	{
		// SceneBase にキャストして DrawShadow を呼ぶ
		if (auto* base = dynamic_cast<SceneBase*>(currentIt->second.scene.get()))
		{
			base->DrawShadow();
		}
	}
}




float SceneManager::GetLoadProgress() const
{
	if (!m_IsLoading) return 1.0f;
	if (!m_NextSceneId) return 0.0f;

	auto it = m_Scenes.find(*m_NextSceneId);
	if (it != m_Scenes.end() && it->second.scene) {
		return it->second.scene->GetLoadProgress();
	}
	return 0.0f;
}


IScene* SceneManager::GetCurrentScene()
{
	auto it = find_or_end(m_Scenes, m_CurrentSceneId);
	return (it != m_Scenes.end()) ? it->second.scene.get() : nullptr;
}

const IScene* SceneManager::GetCurrentScene() const
{
	auto it = m_CurrentSceneId ? m_Scenes.find(*m_CurrentSceneId) : m_Scenes.end();
	return (it != m_Scenes.end()) ? it->second.scene.get() : nullptr;
}


void SceneManager::ProcessLoadingQueue()
{
	if (m_LoadingQueue.empty()) {
		return;
	}

	auto& task = m_LoadingQueue.front();
	task->Execute();
	m_LoadingQueue.pop();
}

void SceneManager::UpdateTransition(float deltaTime)
{
	if (!m_Transition) return;

	m_Transition->Update(deltaTime);//トランジションの更新

	switch (m_Transition->GetPhase())//フェーズごとの処理
	{

	case SceneTransition::Phase::FadeOut:
		
		// フェードアウトが完了したらロードフェーズに移行
		if (m_Transition->GetProgress() >= 1.0f)
		{
			// 現在のシーンを検索
			auto currentIt = find_or_end(m_Scenes, m_CurrentSceneId);

			// 現在のシーンが存在すれば非アクティブ化してアンロード
			if (currentIt != m_Scenes.end() && currentIt->second.scene)
			{
				currentIt->second.scene->OnDeactivate();

				//シーンを完全にアンロードして破棄する
				currentIt->second.scene->OnUnload(); // オブジェクト削除などはここで行われる
				currentIt->second.scene.reset();     // インスタンス削除
				currentIt->second.state = SceneState::Unloaded;
			}

			//現在のシーンIDを無効化
			m_CurrentSceneId = std::nullopt;

			// ローディングフェーズに移行
			if (auto fade = dynamic_cast<FadeTransition*>(m_Transition.get())) 
			{
				fade->SetPhase(SceneTransition::Phase::Loading);
			}
		}
		break;

	case SceneTransition::Phase::Loading:
	{
		if (!m_NextSceneId) break;
		auto factoryIt = m_SceneFactories.find(*m_NextSceneId);
		if (factoryIt != m_SceneFactories.end())
		{
			// 次のシーンを準備
			auto& nextInfo = m_Scenes[*m_NextSceneId];

			if (!nextInfo.scene)//シーンが未生成なら同期ロード
			{
				nextInfo.scene = factoryIt->second();
				nextInfo.scene->OnLoad();
				nextInfo.scene->OnInit();
			}
			else if (nextInfo.state == SceneState::Loading)
			{
				// バックグラウンドスレッドがまだ実行中 → このフレームは待機、次フレームに持ち越す
				break;
			}
			else if (nextInfo.state == SceneState::Ready)
			{
				nextInfo.scene->OnInit(); //プリロード完了済みならOnInitを呼ぶ
			}

			// 次のシーンをアクティブ化し、現在のシーンとして設定
			nextInfo.scene->OnActivate();
			nextInfo.state = SceneState::Active;
			m_CurrentSceneId = m_NextSceneId;
			m_NextSceneId.reset(); // 次のシーンIDをクリア

			// フェードインフェーズに移行
			if (auto fade = dynamic_cast<FadeTransition*>(m_Transition.get())) {
				fade->SetPhase(SceneTransition::Phase::FadeIn);
			}
		}
	}
	break;

	case SceneTransition::Phase::FadeIn:

		// フェードインが完了したらトランジション完了
		if (m_Transition->IsComplete())
		{
			CompleteTransition();
		}
		break;
		
	}
}

void SceneManager::CompleteTransition()
{
	m_IsTransitioning = false;
	m_Transition.reset();

	if (m_OnSceneChanged) {
		m_OnSceneChanged();
	}
}


// ================================================================================
// DefaultLoadingScreen実装
// ================================================================================

void DefaultLoadingScreen::Update(float deltaTime, float progress)
{
	m_Progress = progress;
	m_AnimationTimer += deltaTime;
}

void DefaultLoadingScreen::Draw()
{
	// ローディング画面の描画
	float rotation = m_AnimationTimer * 2.0f;

	//プログレスバー
	float barWidth = 400.0f * m_Progress;

}

