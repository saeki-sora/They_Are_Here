#pragma once
#include "SceneBase.h"
#include "Object.h"
#include "FadeTransition.h"


class SootObject;
class SootRainEffect;
class TitleEnemy;

// TitleSceneクラス
class TitleScene : public SceneBase
{
private:

	// カメラ演出用の変数
	Vector3 m_StartPos;      // 開始位置
	Vector3 m_StartTarget;   // 開始ターゲット
	Vector3 m_EndPos;        // 終了位置
	Vector3 m_EndTarget;     // 終了ターゲット

	float m_Timer = 0.0f;    // 経過時間タイマー
	float m_Duration = 10.0f; // 移動にかかる時間（秒）

	bool m_IsImageShown = false; // 画像を表示したかどうかのフラグ

	SootRainEffect* m_SootEffect = nullptr; // 煤エフェクト（所有権はEffectManager）

	std::weak_ptr<TitleEnemy> m_TitleEnemy; // タイトル画面に表示する敵キャラへの弱参照

	float m_MenuAnimTimer = 0.0f;// メニューアニメーションタイマー
	float m_MenuAnimDuration = 2.5f;// メニューアニメーション時間


//========================
// タイトルメニュー
//========================
	struct MenuItem
	{
		std::string texturePath;
		Vector3 pos;
		Vector3 baseScale;
		std::function<void()> onDecide;
		bool selectable = true;
	};

	std::vector<MenuItem> m_MenuItems; // メニュー項目一覧
	std::vector<std::weak_ptr<SootObject>> m_MenuObjects; // メニュー項目の表示オブジェクトへの弱参照
	int m_SelectedIndex = 0; // 現在選択中のメニュー項目インデックス

	std::weak_ptr<SootObject> m_CursorObject; // 選択カーソルの表示オブジェクトへの弱参照

	// 見た目調整
	float m_SelectedScaleMul = 1.30f; // 選択中だけ少し拡大
	bool  m_MenuInputEnabled = false; // メニュー入力受付中フラグ

	// カーソルアニメーション用
	float m_CursorAnimTimer = 0.0f;
	void UpdateCursor(float deltaTime);

	void BuildMenuItems(); // メニュー項目を作成
	void SpawnMenuObjects();
	void UpdateMenuVisual();
	void MoveSelection(int delta);

public:

	TitleScene() = default; // コンストラクタ
	~TitleScene() = default; // デストラクタ

	void OnInit() override;
	void OnUpdate(float deltaTime) override;
	void OnDeactivate() override;
	void OnUnload() override;

};

