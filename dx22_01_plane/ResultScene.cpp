#include "ResultScene.h"
#include "Game.h"
#include"VisualObject.h"

// コンストラクタ
ResultScene::ResultScene()
{
	Init();
}

// デストラクタ
ResultScene::~ResultScene()
{
	Uninit();
}

// 初期化
void ResultScene::Init()
{
	auto weak_pt = Game::GetInstance().AddObject<VisualObject>();
	if (auto shared_pt = weak_pt.lock())//有効かどうかチェック
	{
		shared_pt->SetPosition(0.0f, 0.0f, 0.0f);
		shared_pt->SetTexture("assets/texture/background2.png");
		shared_pt->SetScale(1280.0f, 720.0f, 0.0f);
	}
	m_MySceneObjects.emplace_back(weak_pt);

	auto weak_pt2 = Game::GetInstance().AddObject<VisualObject>();
	if (auto shared_pt2 = weak_pt2.lock())//有効かどうかチェック
	{
		shared_pt2->SetPosition(0.0f, 0.0f, 0.0f);
		shared_pt2->SetTexture("assets/texture/resultString.png");
		shared_pt2->SetScale(1280.0f, 720.0f, 0.0f);
		shared_pt2->SetUV(1, 1, 1, 13); // UV座標を設定
	}
	m_MySceneObjects.emplace_back(weak_pt2);
}

// 更新
void ResultScene::Update()
{
	// エンターキーを押してタイトルへ
	if (Input::GetKeyTrigger(VK_RETURN))
	{
		Game::GetInstance().ChangeScene(SceneName::TITLE);
	}
}

// 終了処理
void ResultScene::Uninit()
{
	// このシーンのオブジェクトを削除する
	for (auto& o : m_MySceneObjects) {
		Game::GetInstance().DeleteObject(o);
	}
}


