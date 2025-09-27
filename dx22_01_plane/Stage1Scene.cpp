#include "Stage1Scene.h"
#include "Game.h"
#include "GolfBall.h"
#include"VisualObject.h"
#include"Pole.h"
#include"Block.h"
#include "Player.h"
#include "SkyDome.h"
#include"EffectManager.h"
#include "FadeEffect.h"
#include "SceneManager.h"
#include "ResultScene.h"
#include "FadeTransition.h"

using namespace DirectX::SimpleMath;


//バッググラウンド処理
void Stage1Scene::OnLoad()
{
	m_MakeMap.CreatePlan();
}


// 初期化
void Stage1Scene::OnInit()
{
	auto skyDomePtr = AddObject<SkyDome>();
	if (auto sky = skyDomePtr.lock()) // weak_ptrからshared_ptrを取得
	{
		sky->SetScale(1000.0f, 1000.0f, 1000.0f);
		sky->SetTexture("assets/texture/sky.png");
	}

	m_MakeMap.SpawnObjects(this); //オブジェクト生成
	
}



//更新
void Stage1Scene::OnUpdate(float deltaTime)
{
	//Rキーを押してリザルトへ
	if (Input::GetKeyTrigger(VK_R))
	{
		SceneManager::GetInstance().ChangeScene<ResultScene>(
			std::make_unique<FadeTransition>(1.0f)); // 1秒のフェードで遷移
	}
}

// 終了処理
void Stage1Scene::OnUnload()
{

}


