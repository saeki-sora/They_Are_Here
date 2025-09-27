#pragma once
#include "SceneBase.h"
#include "Object.h"
#include"MakeMap.h"

// Stage1Scenクラス
class Stage1Scene : public SceneBase
{
private:

	MakeMap m_MakeMap; // マップ生成クラス

public:
	Stage1Scene() = default;
	~Stage1Scene() = default;

	void OnInit() override;
	void OnLoad() override;
	void OnUpdate(float deltaTime) override;
	void OnUnload() override;
};

