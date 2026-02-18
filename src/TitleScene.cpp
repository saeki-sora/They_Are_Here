#include "pch.h"
#include "TitleScene.h"
#include "Game.h"
#include"VisualObject.h"
#include "EffectManager.h"
#include "FadeEffect.h"
#include"SceneManager.h"
#include "Stage1Scene.h"
#include "FadeTransition.h"
#include "TitleEnemy.h"
#include "SkyDome.h"
#include "SootObject.h"
#include "SootRainEffect.h"
#include "Application.h"
#include "SoundManager.h"


// 初期化
void TitleScene::OnInit()
{
	Renderer::ClearLights(); // ライトの全削除

	// フラグ類のリセット
	m_IsImageShown = false;
	m_MenuInputEnabled = false;

	// タイマー類のリセット
	m_Timer = 0.0f;
	m_MenuAnimTimer = 0.0f;
	m_CursorAnimTimer = 0.0f;

	// 選択状態のリセット
	m_SelectedIndex = 0;

	m_MenuObjects.clear();
	m_TitleEnemy.reset();
	m_CursorObject.reset();

	// エフェクトの安全策
	if (m_SootEffect != nullptr)
	{
		m_SootEffect->Stop();
		m_SootEffect = nullptr;
	}


	BuildMenuItems();// メニュー項目の構築

	// スカイドームの追加
	auto skyDomePtr = AddObject<SkyDome>();
	if (auto sky = skyDomePtr.lock())
	{
		sky->SetScale(1000.0f, 1000.0f, 1000.0f);
		sky->SetTexture("assets/texture/sky.dds");
	}


	//タイトル用の敵キャラ配置
	auto weak_pt1 = AddObject<TitleEnemy>();
	m_TitleEnemy = weak_pt1;
	if (auto shared_pt1 = weak_pt1.lock())
	{
		shared_pt1->SetPosition(Vector3(-70.0f, -130.0f, -10.0f));
		shared_pt1->SetScale(Vector3(1.0f, 1.0f, 1.0f));
		shared_pt1->SetRotation(Vector3(0.25f, 70.5f, 0.0f));
	}

	Renderer::AddPointLight(
		Vector3(-70.0f, 50.0f, 0.0f),  // 位置
		200.0f,                       // 範囲（光が届く距離）
		Color(0.6f, 0.8f, 2.0f, 1.0f)// 色
	);


	//カメラの移動設定
	Camera& camera = Game::GetInstance().GetMainCamera();
	camera.SetMode(Camera::Mode::Stop);

	//スタート地点
	m_StartPos = Vector3(5.1f, -16.7f, 105.9f);
	m_StartTarget = Vector3(-2.2f, -11.0f, 109.8f);

	//ゴール地点
	m_EndPos = Vector3(5.1f, -16.7f, 105.9f);
	m_EndTarget = Vector3(-3.0f, -12.5f, 102.4f);

	camera.SetLookAt(m_StartPos, m_StartTarget);//最初はスタート地点に合わせる

	// タイマーのリセット
	m_Timer = 0.0f;
	m_Duration = 6.0f; // 6秒で移動


	// Stage1Sceneのプリロードをバックグラウンドスレッドで開始
	SceneManager::GetInstance().PreloadScene<Stage1Scene>();

	//煤エフェクト開始
	m_SootEffect = EffectManager::GetInstance().StartEffect<SootRainEffect>(
		static_cast<int>(Application::GetWidth()),
		static_cast<int>(Application::GetHeight()),
		"assets/texture/Soot_transparent.png",
		200
	);

	SoundManager::GetInstance().PlayBGM("BGM_Title", true);
}

// 更新
void TitleScene::OnUpdate(float deltaTime)
{
	Camera& camera = Game::GetInstance().GetMainCamera();

	//カメラの補間移動処理
	if (m_Timer < m_Duration)
	{
		m_Timer += deltaTime;
		float t = m_Timer / m_Duration;// 経過時間の割合を計算

		if (t > 1.0f) t = 1.0f;// 最大1.0fまで
		t = t * t * (3.0f - 2.0f * t);// スムーズステップ補間

		// 現在の位置とターゲットを計算
		Vector3 currentPos = Vector3::Lerp(m_StartPos, m_EndPos, t);
		Vector3 currentTarget = Vector3::Lerp(m_StartTarget, m_EndTarget, t);

		// カメラに適用
		camera.SetLookAt(currentPos, currentTarget);
	}
	else
	{
		if (!m_IsImageShown)
		{
			SpawnMenuObjects();
			m_MenuInputEnabled = true;
			m_MenuAnimTimer = 0.0f;
			m_IsImageShown = true;
		}
	}


	if (m_MenuInputEnabled && !m_MenuItems.empty())
	{
		// アニメーション中かどうかチェック
		if (m_MenuAnimTimer < m_MenuAnimDuration)
		{
			m_MenuAnimTimer += deltaTime;
		}
		else
		{

			// 上
			if (Input::GetKeyTrigger('W'))
			{
				MoveSelection(-1);
			}

			// 下
			if (Input::GetKeyTrigger('S'))
			{
				MoveSelection(+1);
			}

			// 決定（Space）
			if (Input::GetKeyTrigger(VK_SPACE))
			{
				const int idx = m_SelectedIndex;
				if (0 <= idx && idx < static_cast<int>(m_MenuItems.size()))
				{
					// 選択可能項目かつコールバックがあれば実行
					if (m_MenuItems[idx].selectable && m_MenuItems[idx].onDecide)
					{
						m_MenuItems[idx].onDecide();

						// 連打防止のために入力を無効化
						m_MenuInputEnabled = false;
					}
				}
			}
		}
	}

	// カーソルのアニメーション更新（メニュー入力が有効な時だけ）
	if (m_MenuInputEnabled)
	{
		UpdateCursor(deltaTime);
	}
}



void TitleScene::BuildMenuItems()
{
	m_MenuItems.clear();

	// 0: ゲームスタート
	m_MenuItems.push_back(MenuItem{
		"assets/texture/GameStart.png",
		Vector3(1000.0f, 400.0f, 0.0f),// 位置
		Vector3(500.0f, 200.0f, 1.0f),// 基本スケール
		[this]()
		{
			// 敵に歩行命令
			if (auto enemy = m_TitleEnemy.lock())
			{
				// カメラの位置を取得
				Vector3 camPos = Game::GetInstance().GetMainCamera().GetPosition();

				// 敵をカメラに向かって歩かせる
				enemy->StartScareSequence(camPos);
			}

			//遷移開始
			SceneManager::GetInstance().ChangeScene<Stage1Scene>(
				std::make_unique<FadeTransition>(4.0f));
		}
		});

	// ゲーム終了
	m_MenuItems.push_back(MenuItem{
		"assets/texture/GameEnd.png",
		Vector3(1000.0f, 300.0f, 0.0f),
		Vector3(500.0f, 200.0f, 1.0f),
		[this]()
		{
			HWND hWnd = Application::GetWindow();
			if (hWnd != nullptr)
			{
				PostMessage(hWnd, WM_DESTROY, 0, 0);
			}
		}
		});

	//タイトルロゴ
	m_MenuItems.push_back(MenuItem{
		"assets/texture/TitleLogo.png",
		Vector3(400.0f, 600.0f, 0.0f),
		Vector3(900.0f, 500.0f, 1.0f),
		nullptr,// ロゴは選択不可
		false
		});

}


void TitleScene::SpawnMenuObjects()
{
	m_MenuObjects.clear();
	m_MenuObjects.reserve(m_MenuItems.size());

	for (size_t i = 0; i < m_MenuItems.size(); ++i)
	{
		auto weak_pt = AddObject<SootObject>();
		if (auto obj = weak_pt.lock())
		{
			const auto& item = m_MenuItems[i];
			obj->SetTexture(item.texturePath.c_str());
			obj->SetPosition(item.pos.x, item.pos.y, item.pos.z);
			obj->SetScale(item.baseScale);
			obj->StartAnim(2.5f); // アニメーション開始 (2.5秒かけて出現)
		}
		m_MenuObjects.push_back(weak_pt);
	}

	// カーソルオブジェクトの生成
	auto cursorWeak = AddObject<SootObject>();
	m_CursorObject = cursorWeak;

	if (auto cursor = cursorWeak.lock())
	{
		cursor->SetTexture("assets/texture/Cursor.png");
		cursor->SetScale(Vector3(150.0f, 100.0f, 1.0f));
		cursor->StartAnim(2.5f); // メニューと一緒に出現
	}

	m_SelectedIndex = 0; // デフォルトはゲームスタート
	UpdateMenuVisual();
}


// メニューの見た目更新
void TitleScene::UpdateMenuVisual()
{
	const int count = static_cast<int>(m_MenuItems.size());
	if (count <= 0) return;

	// メニュー項目の拡大処理
	for (int i = 0; i < count; ++i)
	{
		auto obj = m_MenuObjects[i].lock();
		if (!obj) continue;

		Vector3 s = m_MenuItems[i].baseScale;
		if (i == m_SelectedIndex && m_MenuItems[i].selectable)
		{
			s *= m_SelectedScaleMul;
		}
		obj->SetScale(s);
	}

}


void TitleScene::MoveSelection(int delta)
{
	const int count = static_cast<int>(m_MenuItems.size());
	if (count <= 0) return;

	const int prev = m_SelectedIndex;
	int next = m_SelectedIndex;

	// 無限ループ防止用のカウンター
	int loopCount = 0;

	// 次の選択可能な項目が見つかるまでループ
	do
	{
		next = (next + delta + count) % count;
		loopCount++;

		// 全周しても見つからなければ移動しない
		if (loopCount > count) {
			next = prev;
			break;
		}

	} while (!m_MenuItems[next].selectable); // 選択不可ならスキップして次へ

	m_SelectedIndex = next;

	// 変化があった時だけ更新
	if (m_SelectedIndex != prev)
	{
		UpdateMenuVisual();
	}
}



void TitleScene::UpdateCursor(float deltaTime)
{
	// カーソルが生きていて、メニュー項目がある場合のみ実行
	if (m_MenuItems.empty()) return;
	auto cursor = m_CursorObject.lock();
	if (!cursor) return;

	// タイマー更新
	m_CursorAnimTimer += deltaTime;

	// 現在選択中の項目の情報を取得
	const auto& selectedItem = m_MenuItems[m_SelectedIndex];

	//カーソルの位置計算
	float baseOffsetX = (selectedItem.baseScale.x * 0.5f) + 80.0f;
	Vector3 targetPos = selectedItem.pos;
	targetPos.x -= baseOffsetX; // 項目の左側に配置

	// 揺れアニメーションの適用
	float swaySpeed = 5.0f;  // 揺れる速さ
	float swayWidth = 10.0f; // 揺れる幅

	// サイン波で左右に揺らす
	float animOffsetX = sin(m_CursorAnimTimer * swaySpeed) * swayWidth;

	targetPos.x += animOffsetX;

	//座標の適用
	cursor->SetPosition(targetPos);
}



void TitleScene::OnDeactivate()
{
	// エフェクトが生きていれば停止命令を出す
	if (m_SootEffect != nullptr)
	{
		m_SootEffect->Stop();
		m_SootEffect = nullptr;// ポインタクリア
	}

	Renderer::ClearLights();
}



// 終了処理
void TitleScene::OnUnload()
{
	Renderer::ClearLights();//点光源全削除
}



