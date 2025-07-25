#pragma once
#include <iostream>
#include <utility> 
#include <random>

#include "Camera.h"
#include "Input.h"
#include "TitleScene.h"
#include "Stage1Scene.h"
#include "ResultScene.h"
#include "Object.h"
using namespace DirectX::SimpleMath;

// シングルトンパターンのゲームクラス

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

public:

	static Game& GetInstance();

	// コピー禁止
	Game(const Game&) = delete;
	Game& operator=(const Game&) = delete;

	void Init(); // 初期化
	void Update(); // 更新
	void Draw(); // 描画
	void Uninit(); // 終了処理

	void ChangeScene(SceneName sName); // シーンを変更
	void DeleteObject(std::weak_ptr<Object> weak_pt); // オブジェクトを削除する(weak_ptrを受け取る)
	void DeleteAllObject(); // オブジェクトをすべて削除する

	Camera& GetMainCamera() { return *m_MainCamera; } // カメラ取得


	// オブジェクトを追加する
	template<class T>
	std::weak_ptr<T> AddObject()
	{
		// TがObjectの派生クラスであることを確認
		static_assert(std::is_base_of<Object, T>::value, "Game::AddObject T must be an Object");

		auto newObjectPtr = std::make_shared<T>();
		m_Objects.emplace_back(newObjectPtr);
		newObjectPtr->Init();
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
		return newObjectPtr;
	}


	// オブジェクトを取得する
	template<class T> std::vector<T*> GetObjects()
	{
		std::vector<T*> res;
		for (auto& o : m_Objects) {
			// dynamic_castで型チェック
			if (T* derivedObj = dynamic_cast<T*>(o.get())) {
				res.emplace_back(derivedObj);
			}
		}
		return res;
	}
};
