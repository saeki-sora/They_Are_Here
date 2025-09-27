#include "TitleScene.h"
#include "Game.h"
#include"VisualObject.h"
#include "EffectManager.h"
#include "FadeEffect.h"
#include"SceneManager.h"
#include "Stage1Scene.h"
#include "FadeTransition.h"


// 初期化
void TitleScene::OnInit()
{
	//背景画像オブジェクトを作成
	auto weak_pt = AddObject<VisualObject>();

	if (auto shared_pt = weak_pt.lock())//有効かどうかチェック
	{
		shared_pt->SetTexture("assets/texture/DebugTitle.png");
		shared_pt->SetPosition(0.0f, 0.0f, 0.0f);
		shared_pt->SetRotation(0.0f, 0.0f, 0.0f);
		shared_pt->SetScale(1300.0f, 720.0f, 0.0f);

	}

	// Stage1Sceneのプリロードをバックグラウンドスレッドで開始
	SceneManager::GetInstance().PreloadScene<Stage1Scene>();
}

// 更新
void TitleScene::OnUpdate(float deltaTime)
{
	//スペースキーを押してステージ1へ
	if (Input::GetKeyTrigger(VK_SPACE))
	{
		SceneManager::GetInstance().ChangeScene<Stage1Scene>(
			std::make_unique<FadeTransition>(1.0f)); // 1秒のフェードで遷移
	}
}

// 終了処理
void TitleScene::OnUnload()
{

}

