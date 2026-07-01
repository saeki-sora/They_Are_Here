#include "pch.h"
#include "GameClearScene.h"
#include "Input.h"
#include "SceneManager.h"
#include "TitleScene.h"
#include "VisualObject.h"
#include "ResultData.h"

// 数字1桁を Number.png スプライトシートから描画する
static void AddDigit(GameClearScene* scene, int digit, float x, float y)
{
	auto weak = scene->AddObject<VisualObject>();
	if (auto vo = weak.lock())
	{
		vo->SetTexture("assets/texture/Nunber.png");
		vo->SetScale(52.0f, 78.0f, 1.0f);
		vo->SetPosition(x, y, 0.0f);
		vo->SetUV(static_cast<float>(digit + 1), 1.0f, 10.0f, 1.0f);
	}
}

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

	// クリア時間を MM SS の 4桁で表示（コロンなし、桁間に空白）
	int totalSec = std::min(static_cast<int>(ResultData::s_ElapsedTime), 5999); // 最大 99:59
	int minutes = totalSec / 60;
	int seconds = totalSec % 60;

	int digits[4] = {
		(minutes / 10) % 10,
		 minutes % 10,
		(seconds / 10) % 10,
		 seconds % 10
	};

	// MM と SS の間に 20px の視覚的な間隔を入れる
	const float baseX = 205.0f;
	const float baseY = -235.0f;
	const float spacing = 58.0f;
	const float gap = 20.0f; // MM と SS の区切り

	for (int i = 0; i < 4; ++i)
	{
		float extraGap = (i >= 2) ? gap : 0.0f;
		AddDigit(this, digits[i], baseX + i * spacing + extraGap, baseY);
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
