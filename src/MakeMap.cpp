#include "pch.h"
#include "MakeMap.h"
#include "ColliderObject.h"
#include "Block.h"
#include "Pole.h"
#include "FloorBlock.h"
#include "Player.h"
#include "Enemy.h"
#include "Pathfinder.h"
#include "InvisibleItem.h"
#include "StalkerEnemy.h"
#include "GoalKey.h"
#include "loversEnemy.h"
#include "LampObject.h"


// コンストラクタ
MakeMap::MakeMap()
    : gen(rd()),
      dir_dist(0, 3),
      pos_x_dist(0, 30),
      pos_y_dist(0, 30),
      percent_dist(0, 99),
      dir(Up)
{
}



// 難易度設定を適用
// CreatePlan() より前に必ず呼ぶこと。
void MakeMap::Configure(const DifficultyConfig& config)
{
    // マップサイズ（奇数であること。偶数だと迷路アルゴリズムが壊れる）
    assert(config.mapSizeX % 2 == 1 && "MakeMap::Configure: mapSizeX は奇数でなければなりません");
    assert(config.mapSizeY % 2 == 1 && "MakeMap::Configure: mapSizeY は奇数でなければなりません");

    // 上限チェック
    assert(config.mapSizeX <= MAP::Config::MAX_SIZE && "MakeMap::Configure: mapSizeX が MAX_SIZE を超えています");
    assert(config.mapSizeY <= MAP::Config::MAX_SIZE && "MakeMap::Configure: mapSizeY が MAX_SIZE を超えています");

	// 難易度設定をメンバ変数に反映
    m_SizeX             = config.mapSizeX;
    m_SizeY             = config.mapSizeY;
    m_NumInvisibleItems = config.numInvisibleItems;
    m_GoalNearRadius    = static_cast<int>(m_SizeX * MAP::Config::GOAL_NEAR_RATIO);
    m_GoalMidRadius     = static_cast<int>(m_SizeX * MAP::Config::GOAL_MID_RATIO);
    m_EnemyCountOverride= config.enemyCount;
    m_StalkerCount      = config.stalkerCount;
    m_LoverPairCount    = config.loverPairCount;
    m_LampCount         = config.lampCount;

    // マップサイズが変わったので乱数ディストリビューションを再初期化する
    pos_x_dist = std::uniform_int_distribution<int>(0, m_SizeX - 1);
    pos_y_dist = std::uniform_int_distribution<int>(0, m_SizeY - 1);
}


// マッププランを生成
void MakeMap::CreatePlan()
{
    blocks.clear();
    wallCount = 0;

	int maxCells = m_SizeX * m_SizeY;// 最大セル数

	// 生成計画の配列をクリアして必要な容量を予約
    plan.blocks.clear();
    plan.blocks.reserve(maxCells / 2); // 壁は多くても半分程度
    plan.floorBlocks.clear();
    plan.floorBlocks.reserve(maxCells);
    plan.InvisibleItem.clear();
    plan.InvisibleItem.reserve(m_NumInvisibleItems);
    plan.enemy.clear();
    plan.enemy.reserve(MAP::Config::MaxEnemies);
    plan.player.clear();
    plan.start.clear();
    plan.goal.clear();
    plan.lamps.clear();

    Init();           // 配列初期化
    SetWall();        // 迷路壁を生成
    SetLandmarks();   // 広場など特殊地形
    SetFloor();       // 床を配置
    SetObjects();     // スタート・ゴール・プレイヤー・敵の座標決定
    SettingBlocks();  // ブロックを配置（ゴール距離で色分け）
    SetItems();       // アイテムを配置
    SetLamps();       // ランプを配置

    CalculateClearance(); // 壁からの距離マップを計算

}


// プランをもとにシーンへオブジェクトを生成
void MakeMap::SpawnObjects(SceneBase* scene)
{
    // 壁ブロック生成
    for (const auto& blockSpec : plan.blocks)
    {
        auto weakBlock = scene->AddObject<Block>(blockSpec.pos, blockSpec.size, blockSpec.type);
        if (auto sharedBlock = weakBlock.lock())
        {
            sharedBlock->SetGridCoords(blockSpec.gridX, blockSpec.gridY);
        }
    }

    // 床ブロック生成
    for (const auto& floorSpec : plan.floorBlocks)
    {
        scene->AddObject<FloorBlock>(floorSpec.pos, floorSpec.size);
    }

    // ゴール生成
    for (const auto& goalSpec : plan.goal)
    {
        scene->AddObject<Pole>(goalSpec.pos, goalSpec.size);
    }

    // プレイヤー生成
    for (const auto& playerSpec : plan.player)
    {
        auto weakPlayer = scene->AddObject<Player>(playerSpec.pos, playerSpec.size);
        if (auto sharedPlayer = weakPlayer.lock())
        {
            sharedPlayer->SetMap(this);
        }
    }

    // ---- 敵の生成 ----
    auto& enemies   = plan.enemy;
    size_t totalCount = enemies.size();

    if (totalCount == 0) return;

    // 座席番号リストを作成してシャッフル
    std::vector<int> indices(totalCount);
    std::iota(indices.begin(), indices.end(), 0);
    std::shuffle(indices.begin(), indices.end(), gen);

    // 難易度設定の内訳に従って種類を割り当てる
    // シャッフル済みの indices の先頭から Stalker → Lovers → Normal の順に確保する
    std::vector<int> stalkerIndices;
    int loverIndex1 = -1;
    int loverIndex2 = -1;

    int assigned = 0;

    for (int i = 0; i < m_StalkerCount && assigned < static_cast<int>(totalCount); ++i, ++assigned)
        stalkerIndices.push_back(indices[assigned]);

    for (int i = 0; i < m_LoverPairCount && assigned + 1 < static_cast<int>(totalCount); i++)
    {
        loverIndex1 = indices[assigned++];
        loverIndex2 = indices[assigned++];
    }

    std::shared_ptr<loversEnemy> ptrLover1 = nullptr;
    std::shared_ptr<loversEnemy> ptrLover2 = nullptr;

    for (int i = 0; i < static_cast<int>(totalCount); ++i)
    {
		// 座席番号 i に対して、ストーカー役とペア役を割り当てる。
        const auto& spec = enemies[i];
        EnemyType type   = DecideEnemyType(i, stalkerIndices, loverIndex1, loverIndex2);
        auto enemy       = CreateEnemy(scene, type, spec.pos, spec.size);

        if (type == EnemyType::Lover)
        {
            if (!ptrLover1) ptrLover1 = std::dynamic_pointer_cast<loversEnemy>(enemy);
            else            ptrLover2 = std::dynamic_pointer_cast<loversEnemy>(enemy);
        }

        if (i == plan.keyEnemyIndex)
        {
            CreateKey(scene, enemy, type);
        }
    }

    // ペアが揃ったらパートナー登録
    if (ptrLover1 && ptrLover2)
    {
        ptrLover1->SetPartner(ptrLover2);
        ptrLover2->SetPartner(ptrLover1);
    }

    // 透明化アイテム生成
    for (const auto& itemSpec : plan.InvisibleItem)
    {
        scene->AddObject<InvisibleItem>(itemSpec.pos, itemSpec.size);
    }

    // ランプ生成
    const float lampRange = MAP::Config::BLOCK_SIZE * MAP::Config::LampRangeScale;
    const DirectX::SimpleMath::Color flameColor(1.0f, MAP::Config::FlameColorG, MAP::Config::FlameColorB, 1.0f);
    for (const auto& lamp : plan.lamps)
    {
        scene->AddObject<LampObject>(lamp.position, lamp.faceDir, lampRange, flameColor);
    }
}


void MakeMap::Init()
{
    // 使用する範囲を初期化
    for (int i = 0; i < m_SizeX; ++i)
    {
        for (int j = 0; j < m_SizeY; ++j)
        {
            MAP[i][j] = 0;
        }
    }
}


void MakeMap::SetWall()
{
    // セル値の意味：0=通路  1=壁(確定)  2=柱(未処理)  3=壁(作成中)
    for (int x = 0; x <= m_SizeX - 1; x += 2)
    {
        for (int y = 0; y <= m_SizeY - 1; y += 2)
        {
            MAP[x][y] = 2;
        }
    }

    // ---- 外周を壁で囲む ----
    for (int x = 0; x < m_SizeX; ++x)
    {
        MAP[x][0]          = 1; // 上辺
        MAP[x][m_SizeY - 1] = 1; // 下辺
    }
    for (int y = 0; y < m_SizeY; ++y)
    {
        MAP[0][y]          = 1; // 左辺
        MAP[m_SizeX - 1][y] = 1; // 右辺
    }

    // ---- 柱から壁を伸ばして迷路を生成する ----
    bool found = false;
    bool end   = false;
    constexpr int MAX_ATTEMPTS = MAP::Config::MaxWallAttempts;

    while (!end)
    {
        found = false;
        int attempts = 0;

        // 未探索の柱（値 = 2）を探す
        for (int x = 0; x <= m_SizeX - 1; ++x)
        {
            for (int y = 0; y <= m_SizeY - 1; ++y)
            {
                if (MAP[x][y] == 2)
                {
                    NowX = x;
                    NowY = y;
                    dir  = Decide_Direction();
                    found = true;
                    break;
                }
                if (x == m_SizeX - 1 && y == m_SizeY - 1) end = true;
            }
            if (found) break;
        }

        if (!found)
        {
            end = true;
            continue;
        }

        // 壁(1)にぶつかるまで伸ばし続ける。
        while (MAP[NowX][NowY] != 1 && attempts < MAX_ATTEMPTS)
        {
            attempts++;

            if (percent_dist(gen) < MAP::Config::CornerRate)
            {
                dir = Decide_Direction();
            }

            bool moved_this_step = false;

            for (int i = 0; i < 4; ++i)
            {
				direction current_try_dir = (i == 0) ? dir : Decide_Direction();// 最初は前回の方向を優先、以降はランダム

                switch (current_try_dir)
                {
                case Up:
					if (NowX >= 2 && NowX <= m_SizeX - 3 && MAP[NowX - 2][NowY] != 3)// 2マス先が作成中の壁でないことを確認
                    {
						for (int k = NowX; k >= NowX - 2; --k)// 2マス分ループして壁を作成する。ただし、途中で確定壁(1)にぶつかったらそこで止める。
                        {
                            if (MAP[k][NowY] == 1) break;
							else MAP[k][NowY] = 3;// 作成中の壁(3)をセット
                        }
                        NowX -= 2;
                        moved_this_step = true;
                        dir = current_try_dir;
                    }
                    break;

                case Down:
                    if (NowX <= m_SizeX - 3 && NowX >= 2 && MAP[NowX + 2][NowY] != 3)
                    {
                        for (int k = NowX; k <= NowX + 2 && k < m_SizeX - 1; ++k)
                        {
                            if (MAP[k][NowY] == 1) break;
                            else MAP[k][NowY] = 3;
                        }
                        NowX += 2;
                        moved_this_step = true;
                        dir = current_try_dir;
                    }
                    break;

                case Left:
                    if (NowY >= 2 && NowY <= m_SizeY - 3 && MAP[NowX][NowY - 2] != 3)
                    {
                        for (int k = NowY; k >= NowY - 2 && k > 0; --k)
                        {
                            if (MAP[NowX][k] == 1) break;
                            else MAP[NowX][k] = 3;
                        }
                        NowY -= 2;
                        moved_this_step = true;
                        dir = current_try_dir;
                    }
                    break;

                case Right:
                    if (NowY <= m_SizeY - 3 && NowY >= 2 && MAP[NowX][NowY + 2] != 3)
                    {
                        for (int k = NowY; k <= NowY + 2; ++k)
                        {
                            if (MAP[NowX][k] == 1) break;
                            else MAP[NowX][k] = 3;
                        }
                        NowY += 2;
                        moved_this_step = true;
                        dir = current_try_dir;
                    }
                    break;
                }

                if (moved_this_step) break;
            }

            if (!moved_this_step) break;

            assert(attempts < MAX_ATTEMPTS && "MakeMap.cpp: 壁伸ばしの試行回数が上限を超えました");
        }

        // 作成中の壁(3) を壁(1) に確定する
        for (int x = 0; x < m_SizeX; ++x)
        {
            for (int y = 0; y < m_SizeY; ++y)
            {
                if (MAP[x][y] == 3)
                {
                    MAP[x][y] = 1;
                    ++wallCount;
                }
            }
        }
    }
}


// ランダムに4方向のどれかを返すだけ。gen は mutable なので const でも OK
MakeMap::direction MakeMap::Decide_Direction() const
{
    switch (dir_dist(gen))
    {
    case 0: return Up;
    case 1: return Down;
    case 2: return Left;
    case 3: return Right;
    default: return Up;
    }
}


// 上下左右すべてが値3か外周なら、作成中(3)をまとめて確定(1)にする。
// SetWall のループ内で行き詰まった場合のフォールバック用。
void MakeMap::WallCompleteCheck()
{
    bool upBlocked    = (NowX >= 2)          ? (MAP[NowX - 2][NowY] == 3) : true;
    bool downBlocked  = (NowX <= m_SizeX - 3) ? (MAP[NowX + 2][NowY] == 3) : true;
    bool leftBlocked  = (NowY >= 2)          ? (MAP[NowX][NowY - 2] == 3) : true;
    bool rightBlocked = (NowY <= m_SizeY - 3) ? (MAP[NowX][NowY + 2] == 3) : true;

    if (upBlocked && downBlocked && leftBlocked && rightBlocked)
    {
        for (int x = 0; x < m_SizeX; ++x)
        {
            for (int y = 0; y < m_SizeY; ++y)
            {
                if (MAP[x][y] == 3) MAP[x][y] = 1;
            }
        }
    }
}



// スタート・ゴール・プレイヤー・敵の座標を決定して plan に登録する
void MakeMap::SetObjects()
{
    plan.player.clear();
    plan.enemy.clear();
    plan.start.clear();
    plan.goal.clear();
    plan.keyEnemyIndex = -1;

    constexpr int max_attempts = MAP::Config::MaxPlaceAttempts;
    int attempts               = 0;

    const float HALF_BLOCK   = MAP::Config::BLOCK_SIZE / 2.0f;
    // マップ中心はセル数 × ブロックサイズの半分
    const float MAP_CENTER_X = m_SizeX / 2.0f * MAP::Config::BLOCK_SIZE;
    const float MAP_CENTER_Y = m_SizeY / 2.0f * MAP::Config::BLOCK_SIZE;
    const float FLOOR_Y      = -MAP::Config::BLOCK_SIZE / 2.0f;

    while (attempts < max_attempts)
    {
        attempts++;

        int x = pos_x_dist(gen);
        int y = pos_y_dist(gen);
        if (MAP[x][y] != 0) continue;

        // 選んだセルから最もマンハッタン距離が遠いセルを探す
        int max_distance = 0;
        int max_x = x, max_y = y;

        for (int i = 0; i < m_SizeX; ++i)
        {
            for (int j = 0; j < m_SizeY; ++j)
            {
                if (MAP[i][j] == 0)
                {
                    int distance = std::abs(x - i) + std::abs(y - j);
                    if (distance > max_distance)
                    {
                        max_distance = distance;
                        max_x = i;
                        max_y = j;
                    }
                }
            }
        }

        if (x == max_x && y == max_y) continue;

        StartX = x;  StartY = y;
        GoalX  = max_x; GoalY = max_y;

        // 敵の数を決定
        int enemyCount = CalcEnemyCountFromMap();

        // 敵スポーン候補セルを収集
        auto spawnCells = CollectEnemySpawnCells();

        if (enemyCount <= 0 || spawnCells.empty())
        {
            assert(false && "MakeMap.cpp: 有効な敵スポーンセルがありません");
            return;
        }

        // 各候補にスコアを付けてソート
        struct EnemyCandidate { int x, y; float score; };
        std::vector<EnemyCandidate> candidates;
        candidates.reserve(spawnCells.size());

        for (const auto& c : spawnCells)
        {
            candidates.push_back({ c.x, c.y, CalcEnemyScore(c.x, c.y) });
        }

        int sortCount = static_cast<int>(candidates.size());
        std::partial_sort(candidates.begin(), candidates.begin() + sortCount, candidates.end(),
            [](const EnemyCandidate& a, const EnemyCandidate& b)
            { return a.score > b.score; });

        enemyCount = std::min(enemyCount, static_cast<int>(candidates.size()));

        // 近すぎる敵を除きながら配置リストを作成
        std::vector<GridCoord> placed;
        placed.reserve(enemyCount);

        for (int ci = 0; ci < sortCount; ++ci)
        {
            if (static_cast<int>(placed.size()) >= enemyCount) break;

            const auto& cand = candidates[ci];
            bool tooClose = false;
            for (const auto& p : placed)
            {
                if (std::abs(p.x - cand.x) + std::abs(p.y - cand.y) < MAP::Config::EnemyMinSeparation)
                {
                    tooClose = true;
                    break;
                }
            }
            if (!tooClose) placed.push_back({ cand.x, cand.y });
        }

        // 配置リストを plan.enemy に登録
        if (!placed.empty())
        {
            EnemyStartX = placed[0].x;
            EnemyStartY = placed[0].y;

            for (const auto& cell : placed)
            {
                Vector3 e_pos;
                e_pos.x = HALF_BLOCK + MAP_CENTER_X - MAP::Config::BLOCK_SIZE * cell.x;
                e_pos.y = FLOOR_Y;
                e_pos.z = -(HALF_BLOCK - MAP_CENTER_Y + MAP::Config::BLOCK_SIZE * cell.y);

                plan.enemy.push_back(ObjectSpec{ cell.x, cell.y, e_pos, Vector3(MAP::Config::EnemySize, MAP::Config::EnemySize, MAP::Config::EnemySize) });
            }
        }
        else
        {
            // 全候補が近すぎる場合のフォールバック（1 体だけ置く）
            if (!candidates.empty())
            {
                const auto& c = candidates.front();
                EnemyStartX = c.x;
                EnemyStartY = c.y;

                Vector3 e_pos;
                e_pos.x = HALF_BLOCK + MAP_CENTER_X - MAP::Config::BLOCK_SIZE * c.x;
                e_pos.y = FLOOR_Y;
                e_pos.z = -(HALF_BLOCK - MAP_CENTER_Y + MAP::Config::BLOCK_SIZE * c.y);

                plan.enemy.push_back(ObjectSpec{ c.x, c.y, e_pos, Vector3(MAP::Config::EnemyFallbackSize, MAP::Config::EnemyFallbackSize, MAP::Config::EnemyFallbackSize) });
                //std::cout << "[Enemy] Fallback: 1 体だけ配置\n";
            }
        }

        // カギを持つ敵をランダムに決定
        if (!plan.enemy.empty())
        {
            std::uniform_int_distribution<int> dist(0, static_cast<int>(plan.enemy.size()) - 1);
            plan.keyEnemyIndex = dist(gen);
        }
        else
        {
            plan.keyEnemyIndex = -1;
        }

        // プレイヤー配置
        Vector3 p_pos;
        p_pos.x = HALF_BLOCK + MAP_CENTER_X - MAP::Config::BLOCK_SIZE * StartX;
        p_pos.y = 0.0f;
        p_pos.z = -(HALF_BLOCK - MAP_CENTER_Y + MAP::Config::BLOCK_SIZE * StartY);
        plan.player.push_back(ObjectSpec{ StartX, StartY, p_pos, Vector3(MAP::Config::PlayerSizeXZ, MAP::Config::PlayerSizeY, MAP::Config::PlayerSizeXZ) });

        // ゴール配置
        Vector3 g_pos;
        g_pos.x = HALF_BLOCK + MAP_CENTER_X - MAP::Config::BLOCK_SIZE * GoalX;
        g_pos.y = 0.0f;
        g_pos.z = -(HALF_BLOCK - MAP_CENTER_Y + MAP::Config::BLOCK_SIZE * GoalY);
        Vector3 g_size = {
            MAP::Config::BLOCK_SIZE / 2.0f,
            MAP::Config::BLOCK_SIZE * 2.0f,
            MAP::Config::BLOCK_SIZE / 2.0f
        };
        plan.goal.push_back(ObjectSpec{ GoalX, GoalY, g_pos, g_size });

        break; // 配置完了
    }

    assert(attempts < max_attempts && "MakeMap.cpp: スタート/ゴール配置に失敗しました");
}


void MakeMap::ResizeBlocks(const std::uint16_t count)
{
    blocks.clear();
    blocks.shrink_to_fit();
    blocks.reserve(count);
}




// ブロック配置。ゴールからの距離で色分けするため、ここでブロックリストを作る。
void MakeMap::SettingBlocks()
{
    const float HALF_BLOCK   = MAP::Config::BLOCK_SIZE / 2.0f;
    const float MAP_CENTER_X = m_SizeX / 2.0f * MAP::Config::BLOCK_SIZE;
    const float MAP_CENTER_Y = m_SizeY / 2.0f * MAP::Config::BLOCK_SIZE;
    DirectX::SimpleMath::Vector3 blockSize(HALF_BLOCK, MAP::Config::BLOCK_SIZE, HALF_BLOCK);

    plan.blocks.clear();

    for (int x = 0; x < m_SizeX; ++x)
    {
        for (int y = 0; y < m_SizeY; ++y)
        {
            if ((x == StartX && y == StartY) ||
                (x == GoalX  && y == GoalY)  ||
                MAP[x][y] == 0)
            {
                continue;
            }

            // ゴールからのマンハッタン距離で色を決定
            // m_GoalNearRadius / m_GoalMidRadius は難易度によって異なる
            int distGoal = std::abs(GoalX - x) + std::abs(GoalY - y);
            BlockType type;
            if      (distGoal <= m_GoalNearRadius) type = BlockType::Normal;
            else if (distGoal <= m_GoalMidRadius)  type = BlockType::MidGoal;
            else                                   type = BlockType::FarGoal;

            float posX = HALF_BLOCK + MAP_CENTER_X - MAP::Config::BLOCK_SIZE * x;
            float posY = 0.0f;
            float posZ = -(HALF_BLOCK - MAP_CENTER_Y + MAP::Config::BLOCK_SIZE * y);

            plan.blocks.push_back(ObjectSpec{ x, y, { posX, posY, posZ }, blockSize, type });
        }
    }
}




// 床は全セルに配置する。壁セルも床がある前提でブロックを置いているため。
void MakeMap::SetFloor()
{
    plan.floorBlocks.clear();

    const float HALF_BLOCK   = MAP::Config::BLOCK_SIZE / 2.0f;
    const float BLOCKSIZE    = MAP::Config::BLOCK_SIZE;
    const float MAP_CENTER_X = m_SizeX / 2.0f * BLOCKSIZE;
    const float MAP_CENTER_Y = m_SizeY / 2.0f * BLOCKSIZE;

    DirectX::SimpleMath::Vector3 floorSize(HALF_BLOCK, MAP::Config::FloorThickness, HALF_BLOCK);

    for (int x = 0; x < m_SizeX; ++x)
    {
        for (int y = 0; y < m_SizeY; ++y)
        {
            float posX = HALF_BLOCK + MAP_CENTER_X - BLOCKSIZE * x;
            float posY = -BLOCKSIZE; // 床は下にずらす
            float posZ = -(HALF_BLOCK - MAP_CENTER_Y + BLOCKSIZE * y);

            plan.floorBlocks.push_back(ObjectSpec{ x, y, { posX, posY, posZ }, floorSize });
        }
    }
}



// アイテム配置
void MakeMap::SetItems()
{
    // 配置数が 0 以下なら何もしない
    if (m_NumInvisibleItems <= 0) return;

    // 通路セルを収集（スタート・ゴールは除外）
    std::vector<GridCoord> walkableCells;
    for (int x = 0; x < m_SizeX; ++x)
    {
        for (int y = 0; y < m_SizeY; ++y)
        {
            if (MAP[x][y] != 0) continue;
            if ((x == StartX && y == StartY) || (x == GoalX && y == GoalY)) continue;
            walkableCells.push_back({ x, y });
        }
    }

    if (walkableCells.empty())
    {
        //std::cout << "アイテム配置場所がないです\n";
        return;
    }

    std::shuffle(walkableCells.begin(), walkableCells.end(), gen);

    const float HALF_BLOCK   = MAP::Config::BLOCK_SIZE / 2.0f;
    const float MAP_CENTER_X = m_SizeX / 2.0f * MAP::Config::BLOCK_SIZE;
    const float MAP_CENTER_Y = m_SizeY / 2.0f * MAP::Config::BLOCK_SIZE;
    Vector3 itemSize(HALF_BLOCK, HALF_BLOCK, HALF_BLOCK);

    // m_NumInvisibleItems だけ配置（難易度で変わる）
    int placeCount = std::min(m_NumInvisibleItems, static_cast<int>(walkableCells.size()));
    for (int i = 0; i < placeCount; ++i)
    {
        const GridCoord& cell = walkableCells[i];

        float posX = HALF_BLOCK + MAP_CENTER_X - MAP::Config::BLOCK_SIZE * cell.x;
        float posY = -(MAP::Config::BLOCK_SIZE * MAP::Config::ItemHeightRatio);
        float posZ = -(HALF_BLOCK - MAP_CENTER_Y + MAP::Config::BLOCK_SIZE * cell.y);

        plan.InvisibleItem.push_back(ObjectSpec{ cell.x, cell.y, { posX, posY, posZ }, itemSize });
    }
}


// ランプ（炎点光源）配置
// 壁セルの通路に面した面にランプを設置する。
// バックグラウンドスレッドで呼ばれるため DirectX オブジェクトは触らない。
void MakeMap::SetLamps()
{
    if (m_LampCount <= 0) return;

    const float HALF_BLOCK   = MAP::Config::BLOCK_SIZE / 2.0f;
    const float BLOCKSIZE    = MAP::Config::BLOCK_SIZE;
    const float MAP_CENTER_X = m_SizeX / 2.0f * BLOCKSIZE;
    const float MAP_CENTER_Y = m_SizeY / 2.0f * BLOCKSIZE;

    // 隣接4方向の定義: { dx, dy, faceDir (ワールド座標系) }
    // X軸: gridX増加 → posX減少 → faceDir.x = -1
    // Z軸: gridY増加 → posZ減少 → faceDir.z = -1
    struct FaceInfo { int dx; int dy; DirectX::SimpleMath::Vector3 dir; };
    const FaceInfo faces[4] = {
        {  1,  0, {-1.0f, 0.0f,  0.0f} }, // grid+X 方向
        { -1,  0, { 1.0f, 0.0f,  0.0f} }, // grid-X 方向
        {  0,  1, { 0.0f, 0.0f, -1.0f} }, // grid+Y 方向
        {  0, -1, { 0.0f, 0.0f,  1.0f} }, // grid-Y 方向
    };

    // 候補リスト: 通路に面した壁面を収集
    std::vector<LampSpec> candidates;
    candidates.reserve(m_SizeX * m_SizeY);

    for (int x = 0; x < m_SizeX; ++x)
    {
        for (int y = 0; y < m_SizeY; ++y)
        {
            if (MAP[x][y] == 0) continue; // 通路セルはスキップ

            // 壁セルのワールド座標（中心）
            float wallX = HALF_BLOCK + MAP_CENTER_X - BLOCKSIZE * x;
            float wallZ = -(HALF_BLOCK - MAP_CENTER_Y + BLOCKSIZE * y);

            for (const auto& face : faces)
            {
                int nx = x + face.dx;
                int ny = y + face.dy;

                if (nx < 0 || nx >= m_SizeX || ny < 0 || ny >= m_SizeY) continue;
                if (MAP[nx][ny] != 0) continue; // 隣が通路でなければスキップ

                // 壁面から通路側にオフセットした位置（壁中段の高さ）
                LampSpec spec;
                spec.faceDir = face.dir;
                spec.position = {
                    wallX + face.dir.x * HALF_BLOCK,
                    BLOCKSIZE * MAP::Config::LampHeightRatio,
                    wallZ + face.dir.z * HALF_BLOCK
                };
                candidates.push_back(spec);
            }
        }
    }

    // ランダムシャッフルして先頭 m_LampCount 個を採用
    std::shuffle(candidates.begin(), candidates.end(), gen);
    int placeCount = std::min(m_LampCount, static_cast<int>(candidates.size()));
    plan.lamps.assign(candidates.begin(), candidates.begin() + placeCount);
}


void MakeMap::SetLandmarks()
{
    SetPark();
}




// 円形広場を作る。壁生成後に呼ぶことで、できた迷路の壁を一部取り除いて広場にする。
void MakeMap::SetPark()
{
	// 広場の半径は、マップサイズに応じて自動決定する。小さいマップでは半径1、大きいマップではもっと大きくなる。
    const int minDim = std::min(m_SizeX, m_SizeY);
    int radius = std::max(1, minDim / MAP::Config::SquareSize);

    int minCx = radius + 1;
    int maxCx = m_SizeX - 2 - radius;
    int minCy = radius + 1;
    int maxCy = m_SizeY - 2 - radius;

    if (minCx > maxCx || minCy > maxCy) return;

    std::uniform_int_distribution<int> cxDist(minCx, maxCx);
    std::uniform_int_distribution<int> cyDist(minCy, maxCy);

    int cx = cxDist(gen);
    int cy = cyDist(gen);

    plazaInfo.cx     = cx;
    plazaInfo.cy     = cy;
    plazaInfo.radius = radius;

    const int r2 = radius * radius;

    for (int x = 1; x < m_SizeX - 1; ++x)
    {
        for (int y = 1; y < m_SizeY - 1; ++y)
        {
            int dx = x - cx;
            int dy = y - cy;
            if (dx * dx + dy * dy <= r2)
            {
                if (x == 0 || x == m_SizeX - 1 || y == 0 || y == m_SizeY - 1) continue;
                if (MAP[x][y] == 1)
                {
                    MAP[x][y] = 0;
                    if (wallCount > 0) --wallCount;
                }
            }
        }
    }
}



// 壁を破壊する。SetPark() などで壁を取り除くときに呼ぶ。
void MakeMap::DestroyWall(int x, int y)
{
    if (x < 0 || y < 0 || x >= m_SizeX || y >= m_SizeY) return;
    if (MAP[x][y] == 0) return;
    MAP[x][y] = 0;
}



// マップから敵の数を計算する。難易度設定で m_EnemyCountOverride が 0 の場合に呼ばれる。
int MakeMap::CalcEnemyCountFromMap() const
{
    // ---- 敵数が固定指定されている場合はそのまま返す ----
    // m_EnemyCountOverride > 0 = DifficultyConfig で明示的に指定された数
    // m_EnemyCountOverride == 0 = 通路セル密度から自動計算
    if (m_EnemyCountOverride > 0)
    {
        return m_EnemyCountOverride;
    }

    // 通路セル数を数える
    int walkableCount = 0;
    for (int x = 0; x < m_SizeX; ++x)
    {
        for (int y = 0; y < m_SizeY; ++y)
        {
            if (MAP[x][y] == 0) ++walkableCount;
        }
    }

    // 密度 × 通路数で敵数を決定し、最小・最大でクランプ
    int enemyCount = static_cast<int>(walkableCount * MAP::Config::EnemyDensity);
    enemyCount = std::clamp(enemyCount, MAP::Config::MinEnemies, MAP::Config::MaxEnemies);

    return enemyCount;
}



// 敵スポーンに適したセルを収集する。スタート・ゴールから一定距離以上離れている必要がある。
std::vector<GridCoord> MakeMap::CollectEnemySpawnCells() const
{
    std::vector<GridCoord> cells;
    const int SAFE_RADIUS = MAP::Config::EnemySpawnSafeRadius;

    for (int x = 0; x < m_SizeX; ++x)
    {
        for (int y = 0; y < m_SizeY; ++y)
        {
            if (MAP[x][y] != 0) continue;
            if ((x == StartX && y == StartY) || (x == GoalX && y == GoalY)) continue;// スタート・ゴールから一定距離以上離れている必要がある

			// スタートとゴールからのマンハッタン距離を計算して、どちらも SAFE_RADIUS 以上離れているセルだけを候補にする
            int distStart = std::abs(StartX - x) + std::abs(StartY - y);
            int distGoal  = std::abs(GoalX  - x) + std::abs(GoalY  - y);
            if (distStart < SAFE_RADIUS || distGoal < SAFE_RADIUS) continue;

            cells.push_back({ x, y });
        }
    }
    return cells;
}



// 敵スポーン候補セルにスコアを付ける。スコアが高いほど「プレイヤーからもゴールからも遠い」良いスポーン位置。
void MakeMap::CalculateClearance()
{
    // 多始点BFS：全壁セルを起点に距離を伝播させる
    const float unit       = MAP::Config::BLOCK_SIZE;
    constexpr float kUnreached = 1e9f;

    // 初期化
    for (int x = 0; x < m_SizeX; ++x)
    {
        for (int y = 0; y < m_SizeY; ++y)
        {
            m_ClearanceMap[x][y] = (MAP[x][y] != 0) ? 0.0f : kUnreached;
        }
    }
        
    // 全壁セルをキューに積む
    std::queue<GridCoord> q;
    for (int x = 0; x < m_SizeX; ++x)
    {
        for (int y = 0; y < m_SizeY; ++y)
        {
            if (MAP[x][y] != 0) q.push({ x, y });

        }
    }

	// 4方向のオフセット
    const int dx[] = { 1, -1, 0, 0 };
    const int dy[] = { 0,  0, 1,-1 };

	// BFS で距離を伝播させる
    while (!q.empty())
    {
		auto [cx, cy] = q.front(); q.pop();//キューの先頭からセルを取り出す
		float nextDist = m_ClearanceMap[cx][cy] + unit;// 現在のセルからの距離にブロックサイズを加算して、隣接セルの距離候補とする

		// 隣接セルを4方向チェック
        for (int d = 0; d < 4; ++d)
        {
            int nx = cx + dx[d], ny = cy + dy[d];

			if (nx < 0 || nx >= m_SizeX || ny < 0 || ny >= m_SizeY) continue;// マップ外はスキップ

			//今から伝えたい距離 nextDistが隣接セルの現在の距離より小さい場合、隣接セルの距離を更新してキューに追加する
            if (nextDist < m_ClearanceMap[nx][ny])
            {
                m_ClearanceMap[nx][ny] = nextDist;
                q.push({ nx, ny });
            }
        }
    }
}



// 敵のタイプを決定する。index は敵の配置順（0 から始まる）。stalkerIndices は Stalker 役の座席番号リスト。
MakeMap::EnemyType MakeMap::DecideEnemyType(int index, const std::vector<int>& stalkerIndices, int loverIdx1, int loverIdx2) const
{
    for (int si : stalkerIndices)
        if (index == si) return EnemyType::Stalker;
    if (index == loverIdx1 || index == loverIdx2) return EnemyType::Lover;
    return EnemyType::Normal;
}



// 敵を生成してシーンに追加する。タイプによって生成クラスが異なる。生成後にマップへのポインタをセットする。
std::shared_ptr<ColliderObject> MakeMap::CreateEnemy(
    SceneBase* scene, EnemyType type,
    const DirectX::SimpleMath::Vector3& pos,
    const DirectX::SimpleMath::Vector3& size)
{
    std::shared_ptr<ColliderObject> createdEnemy = nullptr;

    switch (type)
    {
    case EnemyType::Stalker:
    {
        auto weak = scene->AddObject<StalkerEnemy>(pos, size);
        if (auto shared = weak.lock()) createdEnemy = shared;
        break;
    }
    case EnemyType::Lover:
    {
        auto weak = scene->AddObject<loversEnemy>(pos, size);
        if (auto shared = weak.lock()) createdEnemy = shared;
        break;
    }
    case EnemyType::Normal:
    default:
    {
        auto weak = scene->AddObject<Enemy>(pos, size);
        if (auto shared = weak.lock()) createdEnemy = shared;
        break;
    }
    }

    if (createdEnemy)
    {
        if      (auto e = std::dynamic_pointer_cast<Enemy>(createdEnemy))       e->SetMap(this);
        else if (auto s = std::dynamic_pointer_cast<StalkerEnemy>(createdEnemy)) s->SetMap(this);
        else if (auto l = std::dynamic_pointer_cast<loversEnemy>(createdEnemy))  l->SetMap(this);
    }

    return createdEnemy;
}




// カギを生成してシーンに追加する。targetEnemy はカギを持つ敵のポインタ。
void MakeMap::CreateKey(SceneBase* scene, std::shared_ptr<ColliderObject> targetEnemy, EnemyType type)
{
    if (!targetEnemy) return;

    const DirectX::SimpleMath::Vector3 keySize(MAP::Config::KeySize, MAP::Config::KeySize, MAP::Config::KeySize);

    auto weakKey = scene->AddObject<GoalKey>(targetEnemy->GetPosition(), keySize);
    if (auto key = weakKey.lock())
    {
        key->SetTarget(targetEnemy);

#ifdef _DEBUG
        const char* typeName = "Unknown";
        switch (type)
        {
        case EnemyType::Normal:  typeName = "Normal (通常敵)";  break;
        case EnemyType::Stalker: typeName = "Stalker (ストーカー)"; break;
        case EnemyType::Lover:   typeName = "Lover (恋人)";    break;
        }
        std::cout << "==========================================\n";
        std::cout << "[MakeMap] Key Attached info:\n";
        std::cout << " - Target Type : " << typeName << "\n";
        std::cout << " - Position    : ("
            << targetEnemy->GetPosition().x << ", "
            << targetEnemy->GetPosition().z << ")\n";
        std::cout << "==========================================\n";
#endif
    }
}


// スコアが高いほど「プレイヤーからもゴールからも遠い」良いスポーン位置。
// ゴール側はウェイトをかけて、ゴール近くに敵が溜まりすぎないように調整している
float MakeMap::CalcEnemyScore(int x, int y) const
{
    int distFromPlayer = std::abs(StartX - x) + std::abs(StartY - y);
    int distFromGoal   = std::abs(GoalX  - x) + std::abs(GoalY  - y);
    float adjustedGoalDist = static_cast<float>(distFromGoal) * MAP::Config::EnemyGoalDistWeight;
    return std::min(static_cast<float>(distFromPlayer), adjustedGoalDist);
}


bool MakeMap::IsWalkable(int x, int y) const
{
    if (x < 0 || y < 0 || x >= m_SizeX || y >= m_SizeY) return false;
    return (MAP[x][y] == 0);
}


float MakeMap::GetClearance(int x, int y) const
{
    if (x < 0 || y < 0 || x >= m_SizeX || y >= m_SizeY) return 0.0f;
    return m_ClearanceMap[x][y];
}
