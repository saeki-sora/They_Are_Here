#pragma once
#include "SceneBase.h"
#include "Object.h"

// ResultSceneクラス
class ResultScene : public SceneBase
{
private:

public:
	ResultScene() = default; // コンストラクタ
	~ResultScene() = default; // デストラクタ

	void OnInit() override;
	void OnUpdate(float deltaTime) override;
	void OnUnload() override;

	void SetScore(int c);
};

