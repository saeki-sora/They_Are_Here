#pragma once
#include <iostream>
#include <utility> 
#include <random>
#include <vector>
#include <memory>
#include <algorithm> 
#include <typeindex>
#include <unordered_map>
#include "Camera.h"
#include "Input.h"
#include "TitleScene.h"
#include "Stage1Scene.h"
#include "ResultScene.h"
#include "Object.h"

using namespace DirectX::SimpleMath;

//シングルトンパターンでゲームクラスを定義

// シーン名を列挙型で定義
enum class SceneName {
	TITLE,
	STAGE1,
	RESULT
};

class Game
{
private:
	Game(); // コンストラクタ
	~Game(); // デストラクタ

	std::unique_ptr<Scene> m_Scene; // シーン
	std::vector<std::shared_ptr<Object>> m_Objects; // オブジェクト
	std::unique_ptr<Input> m_Input;  // 入力処理
	std::unique_ptr<Camera> m_MainCamera; // カメラ

	// キャッシュ無効化フラグ（グローバル）
	mutable bool m_CacheValid = false;

	void InvalidateCache() { m_CacheValid = false; } // キャッシュを無効化する
	void CleanupDeadObjects(); // 削除されたオブジェクトのクリーンアップ

public:
	static Game& GetInstance();

	// コピー禁止
	Game(const Game&) = delete;
	Game& operator=(const Game&) = delete;

	void Init();
	void Update(); 
	void Draw();
	void Uninit();
	void ChangeScene(SceneName sName); // シーンを変更

	// オブジェクトの管理
	void DeleteObject(std::weak_ptr<Object> weak_pt); // オブジェクトを削除する
	void DeleteAllObject(); // オブジェクトをすべて削除する
	Camera& GetMainCamera() { return *m_MainCamera; } // カメラ取得

	// オブジェクトを追加する
	template<class T>
	std::weak_ptr<T> AddObject()
	{
		static_assert(std::is_base_of<Object, T>::value, "Game::AddObject T must be an Object");
		auto newObjectPtr = std::make_shared<T>();
		m_Objects.emplace_back(newObjectPtr);
		newObjectPtr->Init();
		InvalidateCache(); // キャッシュ無効化
		return newObjectPtr;
	}

	// オブジェクトを追加する(オーバーロード)
	template<class T>
	std::weak_ptr<T> AddObject(const Vector3& pos, const Vector3& size)
	{
		static_assert(std::is_base_of<Object, T>::value, "Game::AddObject T must be an Object");
		auto newObjectPtr = std::make_shared<T>(pos, size);
		m_Objects.emplace_back(newObjectPtr);
		newObjectPtr->Init();
		InvalidateCache(); // キャッシュ無効化
		return newObjectPtr;
	}

	// オブジェクトを取得する（最初の一つ）
	template<class T>
	std::weak_ptr<T> FindObject()
	{
		for (auto& o : m_Objects) 
		{
			if (auto derivedObj = std::dynamic_pointer_cast<T>(o))
			{
				return derivedObj;
			}
		}
		return std::weak_ptr<T>{}; // ない場合は空のweak_ptrを返す
	}

	// 特定の型のオブジェクトをすべて取得する（キャッシュ付き）
	template<class T>
	std::vector<std::weak_ptr<T>> FindAllObjects() const
	{
		static_assert(std::is_base_of<Object, T>::value, "Game::FindAllObjects T must be an Object");

		// 型ごとの静的キャッシュ
		static std::vector<std::weak_ptr<T>> cachedResults;
		static size_t lastObjectCount = 0;
		static bool lastCacheValid = false;

		// キャッシュが無効、またはオブジェクト数が変わった場合のみ再計算
		if (!lastCacheValid || m_Objects.size() != lastObjectCount || !m_CacheValid)
		{
			cachedResults.clear();

			// 型マッチングして追加
			for (const auto& obj : m_Objects) 
			{
				if (auto derivedObj = std::dynamic_pointer_cast<T>(obj))
				{
					cachedResults.emplace_back(derivedObj);
				}
			}

			// 期限切れのweak_ptrを除去
			cachedResults.erase(
				std::remove_if(cachedResults.begin(), cachedResults.end(),
					[](const std::weak_ptr<T>& weak) {
						return weak.expired();
					}),
				cachedResults.end()
			);

			lastObjectCount = m_Objects.size();
			lastCacheValid = true;
		}

		return cachedResults;// キャッシュされた結果を返す
	}
};