#pragma once
#include <vector>
#include <memory>
#include <any>
#include <atomic>
#include "Object.h"
#include "Game.h"

// ================================================================================
// シーンインターフェース
// ================================================================================
class IScene
{
public:

	virtual ~IScene() = default;

	// ライフサイクル
	virtual void OnLoad() = 0;
	virtual void OnInit() = 0;
	virtual void OnActivate() = 0;
	virtual void OnDeactivate() = 0;
	virtual void OnUnload() = 0;

	virtual void Update(float deltaTime) = 0;
	virtual void Draw() = 0;
	virtual void DrawUI() {}

	// 状態管理
	virtual void OnPause() {}
	virtual void OnResume() {}

	// ロード進捗（0.0〜1.0）
	virtual float GetLoadProgress() const { return 1.0f; }

	// シーン間通信用
	virtual void SetTransitionData(const std::any& data) {}
	virtual std::any GetTransitionData() const { return {}; }
};

// ================================================================================
// 基底シーンクラス
// ================================================================================
class SceneBase : public IScene
{
protected:

	std::vector<std::weak_ptr<Object>> m_SceneObjects;
	std::atomic<float> m_LoadProgress{ 0.0f };
	bool m_IsInitialized = false;

public:
	virtual ~SceneBase();

	// デフォルト実装
	void OnLoad() override;
	void OnInit() override;
	void OnActivate() override;
	void OnDeactivate() override;
	void OnUnload() override;

	//基底クラス用
	void Update(float deltaTime) final;
	void Draw() final;
	void DrawUI() final;

	float GetLoadProgress() const override { return m_LoadProgress; }

	template<class T, typename... Args>
	std::weak_ptr<T> AddObject(Args&&... args)
	{
		// Gameクラスにオブジェクトを生成・所有してもらう
		auto weakObj = Game::GetInstance().AddObject<T>(std::forward<Args>(args)...);

		// 自身の管理リストにも追加する
		if (auto sharedObj = weakObj.lock()) {
			m_SceneObjects.push_back(sharedObj);
		}

		return weakObj;
	}

	void DeleteObject(std::weak_ptr<Object> obj) { Game::GetInstance().DeleteObject(obj); }

protected:

	// 派生クラス用ヘルパー
	void ClearSceneObjects();

	//派生クラスで使う用
	virtual void OnUpdate(float deltaTime) {}
	virtual void OnDraw() {}
	virtual void OnDrawUI() {}
};