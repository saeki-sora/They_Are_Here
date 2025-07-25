#pragma once
#include "Scene.h"
#include "Object.h"
#include"MakeMap.h"

// Stage1Scenクラス
class Stage1Scene : public Scene
{
private:
	std::vector<std::weak_ptr<Object>> m_MySceneObjects; // このシーンのオブジェクト

	void Init(); // 初期化
	void Uninit(); // 終了処理

	int m_Par;
	int m_StrokeCount;//現在の打数

	MakeMap m_MakeMap; // マップ生成クラス

public:
	Stage1Scene(); // コンストラクタ
	~Stage1Scene(); // デストラクタ

	void Update(); // 更新
};

