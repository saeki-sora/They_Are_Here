#pragma once
#include "Object.h"

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
	virtual void OnDeactivate() = 0;// シーンが非アクティブになる直前
	virtual void OnUnload() = 0;// シーンがアンロードされる直前

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

	std::vector<std::shared_ptr<Object>> m_SceneObjects;
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

	// オブジェクトを生成・所有する
	template<class T, typename... Args>
	std::weak_ptr<T> AddObject(Args&&... args)
	{
		static_assert(std::is_base_of<Object, T>::value, "SceneBase::AddObject T must be an Object");

		auto newObjectPtr = std::make_shared<T>(std::forward<Args>(args)...);
		m_SceneObjects.emplace_back(newObjectPtr);
		newObjectPtr->Init();
		return newObjectPtr;
	}

	// オブジェクトを削除する
	void DeleteObject(std::weak_ptr<Object> weak_pt);

	// オブジェクトを取得する（最初の一つ）
	template<class T>
	std::weak_ptr<T> FindObject()
	{
		for (auto& o : m_SceneObjects)
		{
			if (auto derivedObj = std::dynamic_pointer_cast<T>(o))
			{
				return derivedObj;
			}
		}
		return std::weak_ptr<T>{};
	}

	// 特定の型のオブジェクトをすべて取得する
	template<class T>
	std::vector<std::weak_ptr<T>> FindAllObjects() const
	{
		static_assert(std::is_base_of<Object, T>::value, "SceneBase::FindAllObjects T must be an Object");

		std::vector<std::weak_ptr<T>> results;
		for (const auto& obj : m_SceneObjects)
		{
			if (auto derivedObj = std::dynamic_pointer_cast<T>(obj))
			{
				results.emplace_back(derivedObj);
			}
		}
		return results;
	}

protected:

	// 派生クラス用ヘルパー
	void ClearSceneObjects();

	//派生クラスで使う用
	virtual void OnUpdate(float deltaTime) {}
	virtual void OnDraw() {}
	virtual void OnDrawUI() {}
};