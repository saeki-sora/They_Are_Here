#pragma once
#include"SceneBase.h"
class GameOverScene : public SceneBase
{
public:

	GameOverScene() = default; // コンストラクタ
	~GameOverScene() = default; // デストラクタ

	void OnInit() override;
	void OnUpdate(float deltaTime) override;
	void OnUnload() override;
};

