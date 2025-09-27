#include "ResultScene.h"
#include "Game.h"
#include"VisualObject.h"
#include "SceneManager.h"
#include "TitleScene.h"

// 初期化
void ResultScene::OnInit()
{
	//auto weak_pt = Game::GetInstance().AddObject<VisualObject>();
	//if (auto shared_pt = weak_pt.lock())
	//{
	//	shared_pt->SetPosition(0.0f, 0.0f, 0.0f);
	//	shared_pt->SetTexture("assets/texture/Result.png");
	//	shared_pt->SetScale(1280.0f, 720.0f, 0.0f);
	//}
	//m_MySceneObjects.emplace_back(weak_pt);

	auto weak_pt2 = AddObject<VisualObject>();
	if (auto shared_pt = weak_pt2.lock())
	{
		shared_pt->SetPosition(0.0f, 0.0f, 0.0f);
		shared_pt->SetTexture("assets/texture/EnterToTitle.png");
		shared_pt->SetScale(1280.0f, 720.0f, 0.0f);
	}

	

	//auto weak_pt2 = Game::GetInstance().AddObject<VisualObject>();
	//if (auto shared_pt2 = weak_pt2.lock())
	//{
	//	shared_pt2->SetPosition(0.0f, 0.0f, 0.0f);
	//	shared_pt2->SetTexture("assets/texture/resultString.png");
	//	shared_pt2->SetScale(1280.0f, 720.0f, 0.0f);
	//	shared_pt2->SetUV(1, 1, 1, 13);
	//}
	//m_MySceneObjects.emplace_back(weak_pt2);
}

// 更新
void ResultScene::OnUpdate(float deltaTime)
{
	// キーを押してタイトルへ
	if (Input::GetKeyTrigger(VK_SPACE))
	{
		SceneManager::GetInstance().ChangeScene<TitleScene>();
	}
}

// 終了処理
void ResultScene::OnUnload()
{

}


