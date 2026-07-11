#include "pch.h"
#include "GameOverScene.h"
#include "Input.h"
#include "SceneManager.h"
#include "TitleScene.h"
#include "VisualObject.h"
#include "Application.h"

// 初期化
void GameOverScene::OnInit()
{
	// 仮想解像度いっぱいのサイズ（2D描画は仮想解像度基準）
	const float screenW = static_cast<float>(Application::VIRTUAL_WIDTH);
	const float screenH = static_cast<float>(Application::VIRTUAL_HEIGHT);

	auto weak_pt1 = AddObject<VisualObject>();
	if (auto shared_pt = weak_pt1.lock())
	{
		shared_pt->SetPosition(0.0f, 0.0f, 0.0f);
		shared_pt->SetTexture("assets/texture/GameOverBackGround.png");
		shared_pt->SetScale(screenW, screenH, 0.0f);
	}

	auto weak_pt2 = AddObject<VisualObject>();
	if (auto shared_pt = weak_pt2.lock())
	{
		shared_pt->SetPosition(320.0f, -130.0f, 0.0f);
		shared_pt->SetTexture("assets/texture/GameOverUI.png");
		shared_pt->SetScale(screenW, screenH, 0.0f);
	}

	// スペースキーでタイトルへ戻る操作説明
	auto weak_pt3 = AddObject<VisualObject>();
	if (auto shared_pt = weak_pt3.lock())
	{
		shared_pt->SetPosition(0.0f, -280.0f, 0.0f);
		shared_pt->SetTexture("assets/texture/SpaseKey_UI.png");
		shared_pt->SetScale(660.0f, 496.0f, 1.0f);
	}
}

// 更新
void GameOverScene::OnUpdate(float deltaTime)
{
	// キーを押してタイトルへ
	if (Input::GetKeyTrigger(VK_SPACE))
	{
		SceneManager::GetInstance().ChangeScene<TitleScene>(std::make_unique<FadeTransition>(3.0f));
	}
}

// 終了処理
void GameOverScene::OnUnload()
{
	SceneBase::OnUnload();
}
