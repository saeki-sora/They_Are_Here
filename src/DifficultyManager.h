#pragma once
#include "DifficultyConfig.h"
#include "MakeMap.h"
#include <future>
#include <atomic>

// ================================================================================
//難易度選択と、それに対応する MakeMap インスタンスの管理
//タイトル画面起動と同時に、全難易度のマッププランを並列バックグラウンド生成
//難易度選択後、ゲームシーンは GetSelectedMap() から完成済みの MakeMap を受け取る
// ================================================================================


class DifficultyManager
{
public:

	// ---- 難易度プリセット定義 ----
	// ここを変更するだけで全難易度のバランスを一括調整できる
	static constexpr int DIFFICULTY_COUNT = static_cast<int>(Difficulty::Count);

	// ゾーン半径（赤/青）は MakeMap::MAP::Config::GOAL_NEAR_RATIO / GOAL_MID_RATIO で
	// 全難易度共通の比率からマップサイズに応じて自動計算される。
	static constexpr DifficultyConfig CONFIGS [DIFFICULTY_COUNT] =
	{
		//                              mapX mapY enemy stock items lamps stalker loverPair
		{ Difficulty::Easy,   "Easy",   21, 21,  3,   3,   4,   8,   0,   0 },
		//  ↑ 小マップ(21x21)、Normal×3、ストック3、アイテム4個、ランプ8個

		{ Difficulty::Medium, "Medium", 25, 25,  5,   2,   5,  16,   2,   0 },
		//  ↑ 標準マップ(25x25)、Normal×3 + Stalker×2、ストック2、アイテム5個、ランプ16個

		{ Difficulty::Hard,   "Hard",   31, 31,  7,   1,   7,  28,   2,   1 },
		//  ↑ 大マップ(31x31)、Normal×3 + Stalker×2 + Lovers×1ペア、ストック1、アイテム7個、ランプ28個
	};

	// ---- シングルトンアクセス ----
	static DifficultyManager& GetInstance();

	// ---- プリロード ----

	// タイトル画面起動時に呼ぶ。
	// ゲームシーンに到達するころには準備が完了している。
	void PreloadAll();

	// 指定インデックス（0=Easy, 1=Medium, 2=Hard）の準備が完了したか確認
	bool IsReady(int index) const;

	
	bool IsAllReady() const;// 全難易度の準備が完了したか確認
	void WaitAll();// 全スレッドの終了を待機する


	// 難易度選択画面でユーザーが選んだ際に呼ぶ
	void SetSelectedIndex(int index);

	// 現在選択中のインデックスを返す（デフォルト = 1: Medium）
	int GetSelectedIndex() const { return m_SelectedIndex; }


	// Stage1Scene::OnInit() でこれを呼んで SpawnObjects() を実行する
	MakeMap& GetSelectedMap();

	// プレイヤーの最大ストック数など、ゲーム側で参照する設定を返す
	const DifficultyConfig& GetSelectedConfig() const;

private:

	DifficultyManager();
	~DifficultyManager() = default;

	// コピー・ムーブ禁止（シングルトン）
	DifficultyManager(const DifficultyManager&) = delete;
	DifficultyManager& operator=(const DifficultyManager&) = delete;
	DifficultyManager(DifficultyManager&&) = delete;
	DifficultyManager& operator=(DifficultyManager&&) = delete;

	// 難易度ごとの MakeMap インスタンス（3つ並べて管理）
	MakeMap m_Maps[DIFFICULTY_COUNT];

	// 各難易度のバックグラウンドロードタスク
	std::future<void> m_Tasks[DIFFICULTY_COUNT];

	// 各難易度が準備完了したか（スレッドセーフなフラグ）
	std::atomic<bool> m_Ready[DIFFICULTY_COUNT];

	// 現在選択中の難易度インデックス（デフォルト = Medium）
	int m_SelectedIndex = 1;
};
