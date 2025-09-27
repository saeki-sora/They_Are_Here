#pragma once
#include "SceneBase.h"
#include "Object.h"

// TitleSceneクラス
class TitleScene : public SceneBase
{
private:

public:
	TitleScene() = default; // コンストラクタ
	~TitleScene() = default; // デストラクタ

	void OnInit() override;
	void OnUpdate(float deltaTime) override;
	void OnUnload() override;
};

