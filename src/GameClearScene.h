#pragma once
#include"SceneBase.h"

class GameClearScene : public SceneBase
{
public:

	GameClearScene() = default; // コンストラクタ
	~GameClearScene() = default; // デストラクタ

	void OnInit() override;
	void OnUpdate(float deltaTime) override;
	void OnUnload() override;
};

