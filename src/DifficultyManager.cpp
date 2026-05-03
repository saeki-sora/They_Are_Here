#include "pch.h"
#include "DifficultyManager.h"
#include <cassert>

// ================================================================================
// 全難易度の MakeMap を並列バックグラウンドで生成・管理するシングルトン。
// ================================================================================
DifficultyManager::DifficultyManager()
{
	// 難易度ごとの準備完了フラグを初期化
    for (int i = 0; i < DIFFICULTY_COUNT; ++i)
    {
        m_Ready[i].store(false);
    }
}



DifficultyManager& DifficultyManager::GetInstance()
{
    static DifficultyManager instance;
    return instance;
}


// -------------------- プリロード --------------------
void DifficultyManager::PreloadAll()
{
    // 全難易度を並列バックグラウンドでプリロードする
    //   各スレッドは自分専用の m_Maps[i] だけを触る（他スレッドと共有しない）。
    for (int i = 0; i < DIFFICULTY_COUNT; ++i)
    {
        // 既にロード済みまたは実行中なら再起動しない
        if (m_Ready[i].load() || m_Tasks[i].valid())
        {
            continue;
        }

        m_Ready[i].store(false);

		// バックグラウンドでロードタスクを開始
        m_Tasks[i] = std::async(std::launch::async, [this, i]()
        {
            // 難易度設定を MakeMap に適用
            m_Maps[i].Configure(CONFIGS[i]);

            // 迷路生成（壁配置・スタート/ゴール決定・敵/アイテム配置計算）
            m_Maps[i].CreatePlan();

            // 完了フラグを立てる
            m_Ready[i].store(true);
        });
    }
}


// 指定インデックス（0=Easy, 1=Medium, 2=Hard）の準備が完了したか確認
bool DifficultyManager::IsReady(int index) const
{
    if (index < 0 || index >= DIFFICULTY_COUNT) return false;
    return m_Ready[index].load();
}


// 全難易度の準備が完了しているか
bool DifficultyManager::IsAllReady() const
{
    for (int i = 0; i < DIFFICULTY_COUNT; ++i)
    {
        if (!m_Ready[i].load()) return false;
    }
    return true;
}



// 全スレッドの終了を待機する
void DifficultyManager::WaitAll()
{
    // 全スレッドが終わるまでここでブロック。
    for (int i = 0; i < DIFFICULTY_COUNT; ++i)
    {
        if (m_Tasks[i].valid())
        {
            m_Tasks[i].get(); // スレッド終了まで待機
        }
    }
}


// -------------------- 難易度選択 --------------------
void DifficultyManager::SetSelectedIndex(int index)
{
    assert(index >= 0 && index < DIFFICULTY_COUNT && "DifficultyManager: 無効なインデックス");
    m_SelectedIndex = index;
}


// -------------------- 選択中の MakeMap / 設定の取得 --------------------
MakeMap& DifficultyManager::GetSelectedMap()
{
	// ここでロードが終わっていない場合は待機する。
    if (!m_Ready[m_SelectedIndex].load() && m_Tasks[m_SelectedIndex].valid())
    {
        // ロードが終わっていなければここで待つ（フォールバック）
        m_Tasks[m_SelectedIndex].get();
    }
    return m_Maps[m_SelectedIndex];
}


// プレイヤーの最大ストック数など、ゲーム側で参照する設定を返す
const DifficultyConfig& DifficultyManager::GetSelectedConfig() const
{
    return CONFIGS[m_SelectedIndex];
}
