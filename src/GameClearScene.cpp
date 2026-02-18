#include "pch.h"
#include "GameClearScene.h"
#include "Input.h"
#include "SceneManager.h"
#include "TitleScene.h"
#include "VisualObject.h"

// 初期化
void GameClearScene::OnInit()
{
	auto weak_pt4 = AddObject<VisualObject>();
	if (auto shared_pt = weak_pt4.lock())
	{
		shared_pt->SetPosition(0.0f, 0.0f, 0.0f);
		shared_pt->SetTexture("assets/texture/GameClearBackGround.png");
		shared_pt->SetScale(1280.0f, 720.0f, 0.0f);
	}

	auto weak_pt3 = AddObject<VisualObject>();
	if (auto shared_pt = weak_pt3.lock())
	{
		shared_pt->SetPosition(320.0f, -130.0f, 0.0f);
		shared_pt->SetTexture("assets/texture/kanban.png");
		shared_pt->SetScale(650.0f, 480.0f, 0.0f);
	}

	auto weak_pt2 = AddObject<VisualObject>();
	if (auto shared_pt = weak_pt2.lock())
	{
		shared_pt->SetPosition(300.0f, 0.0f, 0.0f);
		shared_pt->SetTexture("assets/texture/GameClear.png");
		shared_pt->SetScale(550.0f, 360.0f, 0.0f);
	}


}

// 更新
void GameClearScene::OnUpdate(float deltaTime)
{
	// キーを押してタイトルへ
	if (Input::GetKeyTrigger(VK_SPACE))
	{
		SceneManager::GetInstance().ChangeScene<TitleScene>(std::make_unique<FadeTransition>(3.0f));
	}
}

// 終了処理
void GameClearScene::OnUnload()
{
	SceneBase::OnUnload();
}


