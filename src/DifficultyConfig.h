#pragma once

// 難易度ごとのパラメータをまとめた設定構造体。

// -------------------- 難易度の種類 --------------------

enum class Difficulty
{
    Easy   = 0,   // 簡単：小マップ・敵少・アイテム多
    Medium = 1,   // 普通：標準設定
    Hard   = 2,   // 難しい：大マップ・敵多・アイテム少

    Count  = 3    // 難易度の総数
};


// -------------------- 難易度ごとの設定パラメータ --------------------

struct DifficultyConfig
{
    // 難易度の識別子
    Difficulty  difficulty;

    // 表示名（UI などで使用）
    const char* name;

    // ---- マップサイズ ----
    // 奇数であること
    int mapSizeX;           // X 方向のセル数
    int mapSizeY;           // Y 方向のセル数

    // ---- 敵 ----
    // 0 を指定すると密度（EnemyDensity）から自動計算。
    // 1 以上を指定するとその数を上書き固定する。
    int enemyCount;

    // ---- 透明化アイテム ----
    int maxInvisibleStock;  // プレイヤーが同時に持てる最大ストック数
    int numInvisibleItems;  // マップに配置するアイテムの個数

    // ---- ランプ（炎点光源）----
    int lampCount;          // マップに配置するランプの個数

    // ---- 敵の種類内訳（stalkerCount + loverPairCount×2 + Normal = enemyCount）----
    int stalkerCount;    // Stalker 敵の数
    int loverPairCount;  // Lovers ペア数（実体は ×2、残りは Normal）

};
