#include "Stage1Scene.h"
#include "Game.h"
#include "GolfBall.h"
#include"VisualObject.h"
#include"Arrow.h"
#include"Pole.h"
#include"Block.h"
#include"Skybox.h"

using namespace DirectX::SimpleMath;

// コンストラクタ
Stage1Scene::Stage1Scene()
{
	Init();
}

// デストラクタ
Stage1Scene::~Stage1Scene()
{
	Uninit();
}

// 初期化
void Stage1Scene::Init()
{
	m_Par = 4;
	m_StrokeCount = 0;

	////スカイドーム配置
	//Skybox* sky = Game::GetInstance().AddObject<Skybox>();
	//sky->SetTexture("assets/texture/sky.dds");
	//m_MySceneObjects.emplace_back(sky);

	m_MakeMap.Create(); // マップ生成クラスの初期化


}

//更新
void Stage1Scene::Update()
{
	 //Rキーを押してリザルトへ
	if (Input::GetKeyTrigger(VK_R))
	{
		Game::GetInstance().ChangeScene(SceneName::RESULT);
	}
}

// 終了処理
void Stage1Scene::Uninit()
{
	// このシーンのオブジェクトを削除する
	for (auto& o : m_MySceneObjects) {
		Game::GetInstance().DeleteObject(o);
	}
}

