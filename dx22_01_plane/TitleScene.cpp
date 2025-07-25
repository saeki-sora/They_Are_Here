#include "TitleScene.h"
#include "Game.h"
#include"VisualObject.h"

// コンストラクタ
TitleScene::TitleScene()
{
	Init();
}

// デストラクタ
TitleScene::~TitleScene()
{
	Uninit();
}

// 初期化
void TitleScene::Init()
{
	//背景画像オブジェクトを作成
	auto weak_pt = Game::GetInstance().AddObject<VisualObject>();

	if (auto shared_pt = weak_pt.lock())//有効かどうかチェック
	{
		shared_pt->SetTexture("assets/texture/DebugTitle.png");
		shared_pt->SetPosition(0.0f, 0.0f, 0.0f);
		shared_pt->SetRotation(0.0f, 0.0f, 0.0f);
		shared_pt->SetScale(1280.0f, 720.0f, 0.0f);

		m_MySceneObjects.emplace_back(weak_pt);
	}
}

// 更新
void TitleScene::Update()
{
	// エンターキーを押してステージ1へ
	if (Input::GetKeyTrigger(VK_RETURN))
	{
		Game::GetInstance().ChangeScene(SceneName::STAGE1);
	}
}

// 終了処理
void TitleScene::Uninit()
{
	// このシーンのオブジェクトを削除する
	for (auto& o : m_MySceneObjects) {
		Game::GetInstance().DeleteObject(o);
	}
}
