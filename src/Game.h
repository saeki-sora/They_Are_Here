#pragma once

#include "Camera.h"
#include "Input.h"
#include "DebugRenderer.h"

using namespace DirectX::SimpleMath;


class Game
{
private:
	Game(); // コンストラクタ
	~Game(); // デストラクタ

	std::unique_ptr<Input> m_Input;  // 入力クラス
	std::unique_ptr<Camera> m_MainCamera; // カメラ

public:
	static Game& GetInstance();

	// インスタンスを取得
	Game(const Game&) = delete;
	Game& operator=(const Game&) = delete;

	void Init();
	void Update(float); 
	void Draw();
	void Uninit();

	Camera& GetMainCamera() { return *m_MainCamera; } // カメラを取得
};
