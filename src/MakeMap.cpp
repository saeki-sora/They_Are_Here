#include "pch.h"
#include "MakeMap.h"
#include "ColliderObject.h"
#include"Block.h"
#include"Pole.h"
#include"FloorBlock.h"
#include "Player.h"
#include "Enemy.h"
#include"Pathfinder.h"
#include"InvisibleItem.h"
#include"SpeedUpItem.h"
#include "StalkerEnemy.h"
#include "GoalKey.h"
#include "loversEnemy.h"



MakeMap::MakeMap()
	: gen(rd()),
	dir_dist(0, 3),
	pos_x_dist(0, MAP::Config::MaxX - 1),
	pos_y_dist(0, MAP::Config::MaxY - 1),
	percent_dist(0, 99),
	dir(Up)
{
}

void MakeMap::CreatePlan()
{

	blocks.clear();
	wallCount = 0;

	int maxCells = MAP::Config::MaxX * MAP::Config::MaxY;

	plan.blocks.clear();
	plan.blocks.reserve(maxCells / 2); // 壁は多くても半分程度

	plan.floorBlocks.clear();
	plan.floorBlocks.reserve(maxCells); // 床は全域

	plan.InvisibleItem.clear();
	plan.InvisibleItem.reserve(MAP::Config::Num_InvisibleItem);

	plan.enemy.clear();
	plan.enemy.reserve(MAP::Config::MaxEnemies);

	plan.player.clear();
	plan.start.clear();
	plan.goal.clear();


	Init();//マップ全体を初期化
	SetWall();//壁を生成
	SetLandmarks();//ランドマークを配置
	SetFloor(); //床を配置
	SetObjects();//スタートとゴールの座標を設定
	SettingBlocks(); // ブロックを配置（ゴール距離で色分け）
	SetItems();//アイテムを配置

	CalculateClearance();//クリアランス計算

#ifdef _DEBUG

	////デバッグ用で出来た迷路をcoutで表示
	//for (int x = 0; x < MAP::Config::MaxX; ++x) {

	//	for (int y = 0; y < MAP::Config::MaxY; ++y) {

	//		std::cout << MAP[x][y] << " ";
	//	}
	//	std::cout << std::endl;
	//}

	////見やすい版
	//std::cout << "\n[Pretty Maze]\n";
	//for (int x = 0; x < MAP::Config::MaxX; ++x) {
	//	for (int y = 0; y < MAP::Config::MaxY; ++y) {
	//		std::cout << (MAP[x][y] == 1 ? "##" : "  ");
	//	}
	//	std::cout << '\n';
	//}

#endif
}


void MakeMap::SpawnObjects(SceneBase* scene)
{
	//壁生成
	for (const auto& blockSpec : plan.blocks)
	{
		auto weakBlock = scene->AddObject<Block>(blockSpec.pos, blockSpec.size, blockSpec.type);

		if (auto sharedBlock = weakBlock.lock())
		{
			// グリッド座標を設定
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
			sharedPlayer->SetMap(this);// 生成したプレイヤーに、MakeMapインスタンスのポインタを渡す
		}
	}

	// === 敵の生成 ===
	auto& enemies = plan.enemy;
	size_t totalCount = enemies.size();// 敵の総数

	if (totalCount == 0) return;// 生成する敵がいなければ終了

	//座席番号リストを作成してシャッフル
	std::vector<int> indices(totalCount);
	std::iota(indices.begin(), indices.end(), 0);
	std::shuffle(indices.begin(), indices.end(), gen);

	// 役職のインデックスを決定
	int stalkerIndex = -1;
	int loverIndex1 = -1;
	int loverIndex2 = -1;

	// 優先度1: 通常敵


	// 優先度2: ストーカー (敵が2体以上いるなら生成)
	if (totalCount >= 2)
	{
		stalkerIndex = indices[1]; // シャッフルされたリストの2番目を使用
	}

	// 優先度3: ペアの敵 (敵が4体以上いるなら生成)
	if (totalCount >= 4)
	{
		loverIndex1 = indices[2];
		loverIndex2 = indices[3];
	}

	// ペアリンク用のポインタ
	std::shared_ptr<loversEnemy> ptrLover1 = nullptr;
	std::shared_ptr<loversEnemy> ptrLover2 = nullptr;

	// ループして生成
	for (int i = 0; i < totalCount; ++i)
	{
		const auto& spec = enemies[i];

		//今の i 番目はどの敵になるべきかを判定
		EnemyType type = DecideEnemyType(i, stalkerIndex, loverIndex1, loverIndex2);

		//敵を生成
		auto enemy = CreateEnemy(scene, type, spec.pos, spec.size);

		//Loverの場合はポインタを保存しておく
		if (type == EnemyType::Lover)
		{
			if (!ptrLover1) ptrLover1 = std::dynamic_pointer_cast<loversEnemy>(enemy);
			else  ptrLover2 = std::dynamic_pointer_cast<loversEnemy>(enemy);
		}

		//カギ生成判定 (カギ担当の番号なら持たせる)
		if (i == plan.keyEnemyIndex)
		{
			CreateKey(scene, enemy, type);
		}
	}

	//ペアが揃っていたらパートナー登録
	if (ptrLover1 && ptrLover2)
	{
		ptrLover1->SetPartner(ptrLover2);
		ptrLover2->SetPartner(ptrLover1);
	}


	//透明化アイテム生成
	for (const auto& itemSpec : plan.InvisibleItem)
	{
		scene->AddObject<InvisibleItem>(itemSpec.pos, itemSpec.size);
	}
}



void MakeMap::Init()
{
	//マップ全体を初期化
	for (int i = 0; i < MAP::Config::MaxX; ++i) {
		for (int j = 0; j < MAP::Config::MaxY; ++j) {
			MAP[i][j] = 0;
		}
	}
}


void MakeMap::SetWall()
{
	//＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝
	//柱を初期化
	//＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝

	for (int x = 0; x <= MAP::Config::MaxX - 1; x += 2) {

		for (int y = 0; y <= MAP::Config::MaxY - 1; y += 2) {

			MAP[x][y] = 2;
		}
	}

	//＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝
	//外周を壁で囲む
	//＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝

	// 上下の壁を設定
	for (int x = 0; x < MAP::Config::MaxX; ++x) {
		MAP[x][0] = 1;               // 上側
		MAP[x][MAP::Config::MaxY - 1] = 1;         // 下側
	}

	// 左右の壁を設定
	for (int y = 0; y < MAP::Config::MaxY; ++y) { // 上下の壁はすでに設定済みなので0からMaxY-1までループ
		MAP[0][y] = 1;                // 左側
		MAP[MAP::Config::MaxX - 1][y] = 1;          // 右側
	}

	//＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝
	//壁を伸ばす
	//＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝

	bool found = false;
	bool end = false;
	const int MAX_ATTEMPTS = 1000; // 最大試行回数

	while (end == false) {

		found = false;
		int attempts = 0; // 試行回数をリセット

		//未探索の柱を探す
		for (int x = 0; x <= MAP::Config::MaxX - 1; ++x)
		{
			for (int y = 0; y <= MAP::Config::MaxY - 1; ++y)
			{
				if (MAP[x][y] == 2)
				{
					NowX = x;
					NowY = y;

					dir = Decide_Direction();//最初の方向をランダムに決める
					found = true;
					break;
				}

				if (x == MAP::Config::MaxX - 1 && y == MAP::Config::MaxY - 1) end = true;
			}

			if (found) break;
		}

		if (!found)//未探索の柱が見つからなかったら終了
		{
			end = true;
			continue; //先頭に戻る
		}

		// 柱(2)が壁(1)または作成中の壁(3)にぶつかるまで壁を伸ばす
		while (MAP[NowX][NowY] != 1 && attempts < MAX_ATTEMPTS)
		{
			attempts++; // 1ステップごとの試行

			//CornerRateの確率で曲がる
			//現在の進行方向をランダムな方向に変える
			if (percent_dist(gen) < MAP::Config::CornerRate)
			{
				dir = Decide_Direction();
			}

			bool moved_this_step = false; // このステップで移動できたか

			//まずdirの方向に移動を試みる
			//もし失敗したらランダムな方向に変えて再試行
			for (int i = 0; i < 4; ++i)
			{
				// 1回目dirを試す。失敗したらランダムな方向を試す
				direction current_try_dir = (i == 0) ? dir : Decide_Direction();

				switch (current_try_dir)
				{
				case Up:
					if (NowX >= 2 && NowX <= MAP::Config::MaxX - 3 && MAP[NowX - 2][NowY] != 3)
					{
						for (int k = NowX; k >= NowX - 2; --k)
						{
							if (MAP[k][NowY] == 1) break;
							else { MAP[k][NowY] = 3; }
						}
						NowX = NowX - 2;
						moved_this_step = true;
						dir = current_try_dir; //成功した方向を保存
					}
					break;

				case Down:
					if (NowX <= MAP::Config::MaxX - 3 && NowX >= 2 && MAP[NowX + 2][NowY] != 3)
					{
						for (int k = NowX; k <= NowX + 2 && k < MAP::Config::MaxX - 1; ++k)
						{
							if (MAP[k][NowY] == 1) break;
							else { MAP[k][NowY] = 3; }
						}
						NowX = NowX + 2;
						moved_this_step = true;
						dir = current_try_dir; //成功した方向を保存
					}
					break;

				case Left:
					if (NowY >= 2 && NowY <= MAP::Config::MaxY - 3 && MAP[NowX][NowY - 2] != 3)
					{
						for (int k = NowY; k >= NowY - 2 && k > 0; --k)
						{
							if (MAP[NowX][k] == 1) break;
							else { MAP[NowX][k] = 3; }
						}
						NowY = NowY - 2;
						moved_this_step = true;
						dir = current_try_dir; //成功した方向を保存
					}
					break;

				case Right:
					if (NowY <= MAP::Config::MaxY - 3 && NowY >= 2 && MAP[NowX][NowY + 2] != 3)
					{
						for (int k = NowY; k <= NowY + 2; ++k)
						{
							if (MAP[NowX][k] == 1) break;
							else { MAP[NowX][k] = 3; }
						}
						NowY = NowY + 2;
						moved_this_step = true;
						dir = current_try_dir; //成功した方向を保存
					}
					break;
				}

				if (moved_this_step) break; // 4回試行ループを抜ける
			}

			//4方向すべて試しても動けなかった場合
			if (!moved_this_step) break;// この柱からの壁伸ばしは終了

			//試行回数が上限に達したらエラー
			assert(attempts < MAX_ATTEMPTS && "MakeMap.cpp: Wall extension failed - too many attempts");
		}

		//作成中の壁(3)を壁(1)に変更
		for (int x = 0; x < MAP::Config::MaxX; ++x)
		{
			for (int y = 0; y < MAP::Config::MaxY; ++y)
			{
				if (MAP[x][y] == 3) {
					MAP[x][y] = 1;
					++wallCount; // 壁の数をカウント
				}
			}
		}
	}
}






//上下左右どちらに壁を作るか決める
MakeMap::direction MakeMap::Decide_Direction() const {

	int dir_value = dir_dist(gen);

	switch (dir_value)
	{
	case 0: return Up; break;
	case 1: return Down; break;
	case 2: return Left; break;
	case 3: return Right; break;
	default: return Up;
	}
}




//上下左右が全て作成中の壁ではないか確認
void MakeMap::WallCompleteCheck() {

	// 境界条件をチェックしてから配列アクセス
	bool upBlocked = (NowX >= 2) ? (MAP[NowX - 2][NowY] == 3) : true;
	bool downBlocked = (NowX <= MAP::Config::MaxX - 3) ? (MAP[NowX + 2][NowY] == 3) : true;
	bool leftBlocked = (NowY >= 2) ? (MAP[NowX][NowY - 2] == 3) : true;
	bool rightBlocked = (NowY <= MAP::Config::MaxY - 3) ? (MAP[NowX][NowY + 2] == 3) : true;


	if (upBlocked && downBlocked && leftBlocked && rightBlocked)
	{
		//作成中の壁を壁に変更
		for (int x = 0; x < MAP::Config::MaxX; ++x)
		{
			for (int y = 0; y < MAP::Config::MaxY; ++y)
			{
				if (MAP[x][y] == 3)
				{
					MAP[x][y] = 1;
				}
			}
		}
	}
}


//＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝
//スタートとゴールの座標を設定
//＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝
void MakeMap::SetObjects()
{
	//一旦全員の情報をクリア
	plan.player.clear();
	plan.enemy.clear();
	plan.start.clear();
	plan.goal.clear();
	plan.enemy.clear();
	plan.keyEnemyIndex = -1;

	// ループの上限を設定
	int max_attempts = 100;
	int attempts = 0;

	//座標変換用の定数
	const float HALF_BLOCK = MAP::Config::BLOCK_SIZE / 2.0f;
	const float MAP_CENTER_X = MAP::Config::MaxX / 2.0f * MAP::Config::BLOCK_SIZE;
	const float MAP_CENTER_Y = MAP::Config::MaxY / 2.0f * MAP::Config::BLOCK_SIZE;
	const float FLOOR_Y = -MAP::Config::BLOCK_SIZE / 2.0f; // 床の高さ


	//MAPの中の0のマスからランダムに一つ選ぶ
	while (attempts < max_attempts)
	{
		attempts++;

		int x = pos_x_dist(gen);
		int y = pos_y_dist(gen);

		if (MAP[x][y] != 0) continue; // 壁ならやり直し

		//選んだ0のマスから最も離れている0のマスを二つ選ぶ
		int max_distance = 0;
		int max_x = x;
		int max_y = y;

		for (int i = 0; i < MAP::Config::MaxX; ++i)
		{
			for (int j = 0; j < MAP::Config::MaxY; ++j)
			{
				if (MAP[i][j] == 0)
				{
					int distance = abs(x - i) + abs(y - j); // マンハッタン距離を計算

					// 距離が最大のマスを更新
					if (distance > max_distance)
					{
						max_distance = distance;
						max_x = i;
						max_y = j;
					}
				}
			}
		}


		// スタートとゴールが同じならやり直し
		if (x == max_x && y == max_y) continue;


		// スタートとゴールを設定
		StartX = x;
		StartY = y;
		GoalX = max_x;
		GoalY = max_y;


		//マップの通路数から敵の数を自動決定
		int enemyCount = CalcEnemyCountFromMap();

		//配置候補セルを収集
		auto spawnCells = CollectEnemySpawnCells();



		if (enemyCount <= 0 || spawnCells.empty())
		{
			assert(false && "MakeMap.cpp: No valid enemy spawn cells available");
			return;
		}

		//各候補セルにスコアを付けてソート
		struct EnemyCandidate
		{
			int   x;
			int   y;
			float score;
		};

		// 候補リスト
		std::vector<EnemyCandidate> candidates;
		candidates.reserve(spawnCells.size());

		// スコア計算
		for (const auto& c : spawnCells)
		{
			float score = CalcEnemyScore(c.x, c.y);
			candidates.push_back(EnemyCandidate{ c.x, c.y, score });
		}

		// スコアでソート（上位だけ必要なので partial_sort で高速化）
		int sortCount = std::min(static_cast<int>(candidates.size()), enemyCount * 3);
		std::partial_sort(candidates.begin(), candidates.begin() + sortCount, candidates.end(),
			[](const EnemyCandidate& a, const EnemyCandidate& b)
			{
				return a.score > b.score; // スコアの高い順
			});

		// 上限調整
		enemyCount = std::min(enemyCount, static_cast<int>(candidates.size()));


		//既に置いた敵との距離を見ながら、順に配置
		std::vector<GridCoord> placed;
		placed.reserve(enemyCount);

		for (int ci = 0; ci < sortCount; ++ci)
		{
			const auto& cand = candidates[ci];
			if (static_cast<int>(placed.size()) >= enemyCount)
				break;

			// 既に置いた敵との距離をチェック
			bool tooClose = false;
			for (const auto& p : placed)
			{
				int dx = std::abs(p.x - cand.x);
				int dy = std::abs(p.y - cand.y);
				int manhattan = dx + dy;

				// 最小距離未満ならスキップ
				if (manhattan < MAP::Config::EnemyMinSeparation)
				{
					tooClose = true;
					break;
				}
			}

			// 既に置いた敵との距離を見ながら、順に配置
			if (tooClose) continue;

			// この候補を採用
			placed.push_back({ cand.x, cand.y });
		}



		// 敵生成情報を登録
		if (!placed.empty())
		{
			// 最初に配置した敵を代表として EnemyStart に保存
			EnemyStartX = placed[0].x;
			EnemyStartY = placed[0].y;

			for (const auto& cell : placed)
			{
				Vector3 e_pos;
				e_pos.x = 0.0f + (HALF_BLOCK + MAP_CENTER_X - (MAP::Config::BLOCK_SIZE * cell.x));
				e_pos.y = FLOOR_Y; // 床の高さ
				e_pos.z = 0.0f - (HALF_BLOCK - MAP_CENTER_Y + (MAP::Config::BLOCK_SIZE * cell.y));

				Vector3 e_size = { 0.3f, 0.3f, 0.3f };

				plan.enemy.push_back(ObjectSpec{ cell.x, cell.y, e_pos, e_size });// 敵配置情報を追加
			}
		}
		else
		{
			// すべて近すぎて置けなかった場合などのフォールバック (スコア1位だけでも置く)
			if (!candidates.empty())
			{
				const auto& c = candidates.front();

				EnemyStartX = c.x;
				EnemyStartY = c.y;

				Vector3 e_pos;
				e_pos.x = 0.0f + (HALF_BLOCK + MAP_CENTER_X - (MAP::Config::BLOCK_SIZE * c.x));
				e_pos.y = FLOOR_Y;
				e_pos.z = 0.0f - (HALF_BLOCK - MAP_CENTER_Y + (MAP::Config::BLOCK_SIZE * c.y));

				Vector3 e_size = { 0.7f, 0.7f, 0.7f };

				plan.enemy.push_back(ObjectSpec{ c.x, c.y, e_pos, e_size });

				std::cout << "[Enemy] Fallback: Placed 1 enemy from top candidate.\n";
			}
		}


		// キー敵のインデックスをランダムに決定
		if (!plan.enemy.empty())
		{
			std::uniform_int_distribution<int> dist(0, static_cast<int>(plan.enemy.size()) - 1);
			plan.keyEnemyIndex = dist(gen); // 0 ～ 敵の数-1 の乱数
		}
		else
		{
			plan.keyEnemyIndex = -1;// 敵がいない場合は-1
		}


		// プレイヤー生成
		Vector3 p_pos;
		p_pos.x = 0.0f + (HALF_BLOCK + MAP_CENTER_X - (MAP::Config::BLOCK_SIZE * StartX));
		p_pos.y = 0.0f;
		p_pos.z = 0.0f - (HALF_BLOCK - MAP_CENTER_Y + (MAP::Config::BLOCK_SIZE * StartY));
		Vector3 p_size = Vector3(1.0f, 0.3f, 1.0f);;
		plan.player.push_back(ObjectSpec{ StartX, StartY,p_pos,p_size });

		// ゴール生成
		Vector3 g_pos;
		g_pos.x = 0.0f + (HALF_BLOCK + MAP_CENTER_X - (MAP::Config::BLOCK_SIZE * GoalX));
		g_pos.y = 0.0f;
		g_pos.z = 0.0f - (HALF_BLOCK - MAP_CENTER_Y + (MAP::Config::BLOCK_SIZE * GoalY));
		Vector3 g_size = { MAP::Config::BLOCK_SIZE / 2.0f , MAP::Config::BLOCK_SIZE * 2.0f, MAP::Config::BLOCK_SIZE / 2.0f };
		plan.goal.push_back(ObjectSpec{ GoalX, GoalY,g_pos,g_size });


		break;// 全員位置決定したらループを抜ける




		// スタートとゴールが設定できなかった場合のフォールバック処理
		assert(attempts < max_attempts && "MakeMap.cpp: Failed to assign start and goal positions");
	}
}



// ブロック配列のリサイズ
void MakeMap::ResizeBlocks(const std::uint16_t count)
{
	blocks.clear();
	blocks.shrink_to_fit();
	blocks.reserve(count);// 壁の数を計算してメモリを事前確保
}


// ブロック配置
void MakeMap::SettingBlocks()
{
	const float HALF_BLOCK = MAP::Config::BLOCK_SIZE / 2.0f;// ブロックの半分のサイズを計算
	const float MAP_CENTER_X = MAP::Config::MaxX / 2.0f * MAP::Config::BLOCK_SIZE;// マップの中心座標を計算
	const float MAP_CENTER_Y = MAP::Config::MaxY / 2.0f * MAP::Config::BLOCK_SIZE;
	DirectX::SimpleMath::Vector3 blockSize(HALF_BLOCK, MAP::Config::BLOCK_SIZE, HALF_BLOCK);// ブロックのサイズを設定

	plan.blocks.clear();

	for (int x = 0; x < MAP::Config::MaxX; ++x) {

		for (int y = 0; y < MAP::Config::MaxY; ++y) {

			// スタート・ゴール・空白はスキップ
			if ((x == StartX && y == StartY) ||
				(x == GoalX && y == GoalY) ||
				(MAP[x][y] == 0))
			{
				continue;
			}


			// ゴールからのマンハッタン距離でブロックタイプを決定
			int distGoal = std::abs(GoalX - x) + std::abs(GoalY - y);
			BlockType type;
			if (distGoal <= MAP::Config::GoalNearRadius)
				type = BlockType::Normal;   // 近距離（赤：デフォルトテクスチャ）
			else if (distGoal <= MAP::Config::GoalMidRadius)
				type = BlockType::MidGoal;  // 中距離（青）
			else
				type = BlockType::FarGoal; // 遠距離（緑）

			// 3D空間での位置計算
			float posX = 0.0f + (HALF_BLOCK + MAP_CENTER_X - (MAP::Config::BLOCK_SIZE * x));
			float posY = 0.0f;
			float posZ = 0.0f - (HALF_BLOCK - MAP_CENTER_Y + (MAP::Config::BLOCK_SIZE * y));

			DirectX::SimpleMath::Vector3 position(posX, posY, posZ);

			plan.blocks.push_back(ObjectSpec{ x, y, position, blockSize, type });

		}
	}
}



void MakeMap::SetFloor()
{
	plan.floorBlocks.clear();

	const float HALF_BLOCK = MAP::Config::BLOCK_SIZE / 2.0f;
	const float BLOCKSIZE = MAP::Config::BLOCK_SIZE;

	// マップ中心
	const float MAP_CENTER_X = MAP::Config::MaxX / 2.0f * BLOCKSIZE;
	const float MAP_CENTER_Y = MAP::Config::MaxY / 2.0f * BLOCKSIZE;

	// 床ブロックのサイズ
	DirectX::SimpleMath::Vector3 floorSize(HALF_BLOCK, 10.0f, HALF_BLOCK);

	for (int x = 0; x < MAP::Config::MaxX; ++x)
	{
		for (int y = 0; y < MAP::Config::MaxY; ++y)
		{

			// 3D空間での位置計算
			float posX = 0.0f + (HALF_BLOCK + MAP_CENTER_X - (BLOCKSIZE * x));
			float posY = -(MAP::Config::BLOCK_SIZE);    //下にずらす
			float posZ = 0.0f - (HALF_BLOCK - MAP_CENTER_Y + (BLOCKSIZE * y));

			DirectX::SimpleMath::Vector3 pos(posX, posY, posZ);

			plan.floorBlocks.push_back(ObjectSpec{ x, y, pos, floorSize });
		}
	}
}



void MakeMap::SetItems()
{
	if (MAP::Config::Num_InvisibleItem <= 0) return;

	//配置可能なセルをすべて集める
	std::vector<GridCoord> walkableCells;
	for (int x = 0; x < MAP::Config::MaxX; ++x) {
		for (int y = 0; y < MAP::Config::MaxY; ++y) {
			if (MAP[x][y] == 0) {

				// プレイヤー、ゴールの位置は除外する
				if ((x == StartX && y == StartY) ||
					(x == GoalX && y == GoalY))
				{
					continue;
				}
				walkableCells.push_back({ x, y });
			}
		}
	}

	//配置する場所がなければ終了
	if (walkableCells.empty()) {
		std::cout << "アイテム配置場所がないです\n";
		return;
	}

	//配置可能なセルリストをシャッフルする
	std::shuffle(walkableCells.begin(), walkableCells.end(), gen);

	//座標変換用の定数 (Set_Start_Goal からコピー)
	const float HALF_BLOCK = MAP::Config::BLOCK_SIZE / 2.0f;
	const float MAP_CENTER_X = MAP::Config::MaxX / 2.0f * MAP::Config::BLOCK_SIZE;
	const float MAP_CENTER_Y = MAP::Config::MaxY / 2.0f * MAP::Config::BLOCK_SIZE;

	//アイテムのデフォルトサイズ
	Vector3 itemSize(HALF_BLOCK, HALF_BLOCK, HALF_BLOCK);

	for (int i = 0; i < MAP::Config::Num_InvisibleItem; i++)
	{
		GridCoord cell = walkableCells[i];

		// ワールド座標に変換
		float i_posX = 0.0f + (HALF_BLOCK + MAP_CENTER_X - (MAP::Config::BLOCK_SIZE * cell.x));
		float i_posY = 0.0f - (MAP::Config::BLOCK_SIZE / 4.0f);
		float i_posZ = 0.0f - (HALF_BLOCK - MAP_CENTER_Y + (MAP::Config::BLOCK_SIZE * cell.y));

		Vector3 itemPos(i_posX, i_posY, i_posZ);

		// planに追加
		plan.InvisibleItem.push_back(ObjectSpec{ cell.x, cell.y, itemPos, itemSize });

	}
}


void MakeMap::SetLandmarks()
{
	SetPark();// 広場を作成
}




void MakeMap::SetPark()
{

	// 広場を作成して、該当セルの壁(1)を通路(0)にする
	// 半径はマップの短辺の１/SquareSize
	const int minDim = std::min(MAP::Config::MaxX, MAP::Config::MaxY);
	int radius = std::max(1, minDim / MAP::Config::SquareSize);

	// 外周の壁に触れないように
	int minCx = radius + 1;
	int maxCx = MAP::Config::MaxX - 2 - radius;
	int minCy = radius + 1;
	int maxCy = MAP::Config::MaxY - 2 - radius;

	// 広場を配置する十分なスペースが無ければ終了
	if (minCx > maxCx || minCy > maxCy) return;

	std::uniform_int_distribution<int> cxDist(minCx, maxCx);
	std::uniform_int_distribution<int> cyDist(minCy, maxCy);

	int cx = cxDist(gen);
	int cy = cyDist(gen);

	// 広場情報を保存
	plazaInfo.cx = cx;
	plazaInfo.cy = cy;
	plazaInfo.radius = radius;

	const int r2 = radius * radius;

	// 円形領域内のセルを通路にする。ただし外周セルは触らない
	for (int x = 1; x < MAP::Config::MaxX - 1; ++x)
	{
		for (int y = 1; y < MAP::Config::MaxY - 1; ++y)
		{
			int dx = x - cx;
			int dy = y - cy;
			if (dx * dx + dy * dy <= r2)
			{
				// 外周に影響を与えないように外周セルはスキップ
				if (x == 0 || x == MAP::Config::MaxX - 1 || y == 0 || y == MAP::Config::MaxY - 1) continue;

				if (MAP[x][y] == 1)
				{
					MAP[x][y] = 0;
					if (wallCount > 0) --wallCount; // 壁カウントを整合させる
				}
			}
		}
	}
}





// 壁を破壊し、グリッドを通路(0)に変更する
void MakeMap::DestroyWall(int x, int y)
{
	// 座標の範囲チェック
	if (x < 0 || y < 0 || x >= MAP::Config::MaxX || y >= MAP::Config::MaxY)
	{
		return;
	}

	if (MAP[x][y] == 0)
	{
		return; // 既に通路
	}

	// 壁(1) を 通路(0) に変更
	MAP[x][y] = 0;
}



int MakeMap::CalcEnemyCountFromMap() const
{
	int walkableCount = 0;

	for (int x = 0; x < MAP::Config::MaxX; ++x)
	{
		for (int y = 0; y < MAP::Config::MaxY; ++y)
		{
			if (MAP[x][y] == 0) // 通路
			{
				++walkableCount;// 通路セル数をカウント
			}
		}
	}

	// 通路セル数に敵密度を掛けて敵数を計算
	int enemyCount = static_cast<int>(walkableCount * MAP::Config::EnemyDensity);
	enemyCount = std::clamp(enemyCount, MAP::Config::MinEnemies, MAP::Config::MaxEnemies);

	return enemyCount;// 敵数を返す
}




// スタート・ゴールから一定距離以上離れた通路セルを収集
std::vector<GridCoord> MakeMap::CollectEnemySpawnCells() const
{
	std::vector<GridCoord> cells;

	const int SAFE_RADIUS = 3; // スタート/ゴールから3マス以内は敵を置かない

	for (int x = 0; x < MAP::Config::MaxX; ++x)
	{
		for (int y = 0; y < MAP::Config::MaxY; ++y)
		{
			if (MAP[x][y] != 0) continue; // 通路以外はNG

			// スタート・ゴールそのものは除外
			if ((x == StartX && y == StartY) ||
				(x == GoalX && y == GoalY))
			{
				continue;
			}

			int distStart = std::abs(StartX - x) + std::abs(StartY - y);
			int distGoal = std::abs(GoalX - x) + std::abs(GoalY - y);

			if (distStart < SAFE_RADIUS || distGoal < SAFE_RADIUS)// スタート・ゴールに近すぎる
			{
				continue;
			}

			cells.push_back({ x, y });
		}
	}

	return cells;
}




// 各セルのクリアランスマップを計算
void MakeMap::CalculateClearance()
{
	const int searchRadius = MAP::Config::ClearanceSearchRadius;

	// 距離の初期値
	const float maxDistValue = (float)searchRadius * MAP::Config::BLOCK_SIZE;

	// 全セルループ
	for (int x = 0; x < MAP::Config::MaxX; ++x)
	{
		for (int y = 0; y < MAP::Config::MaxY; ++y)
		{
			// 壁そのものなら距離0
			if (MAP[x][y] != 0)
			{
				m_ClearanceMap[x][y] = 0.0f;
				continue;
			}

			// 最も近い壁を探す
			// 初期値は探索範囲外の値にしておく
			float minDistance = maxDistValue;
			bool foundWall = false;

			// 自分の周囲 searchRadius マスだけをループする
			int startX = std::max(0, x - searchRadius);
			int endX = std::min((int)MAP::Config::MaxX - 1, x + searchRadius);
			int startY = std::max(0, y - searchRadius);
			int endY = std::min((int)MAP::Config::MaxY - 1, y + searchRadius);

			for (int cx = startX; cx <= endX; ++cx)
			{
				for (int cy = startY; cy <= endY; ++cy)
				{
					// 壁を見つけた時だけ距離計算
					if (MAP[cx][cy] != 0)
					{
						// 差分（絶対値）
						float dx = std::abs((float)(x - cx));
						float dy = std::abs((float)(y - cy));

						//壁の表面までの距離
						float distX = std::max(0.0f, dx - 0.5f);
						float distY = std::max(0.0f, dy - 0.5f);

						// 三平方の定理（正確な斜め距離）
						float dist = std::sqrt(distX * distX + distY * distY);

						// ワールド座標距離に変換
						float worldDist = dist * MAP::Config::BLOCK_SIZE;

						if (worldDist < minDistance)
						{
							minDistance = worldDist;
							foundWall = true;
						}
					}
				}
			}

			// 計算結果を保存
			m_ClearanceMap[x][y] = minDistance;
		}
	}
}



// ----------------------------------------------------------------
// インデックスと役職IDを比較し、生成すべき敵タイプを決定する
// 優先順位: Stalker > Lover > Normal
// ----------------------------------------------------------------
MakeMap::EnemyType MakeMap::DecideEnemyType(int index, int stalkerIdx, int loverIdx1, int loverIdx2) const
{
	if (index == stalkerIdx) return EnemyType::Stalker;
	if (index == loverIdx1 || index == loverIdx2) return EnemyType::Lover;

	return EnemyType::Normal;
}


// ----------------------------------------------------------------
// 指定されたタイプに基づいて敵オブジェクトを生成・初期化する
// ----------------------------------------------------------------
std::shared_ptr<ColliderObject> MakeMap::CreateEnemy(SceneBase* scene, EnemyType type, const DirectX::SimpleMath::Vector3& pos, const DirectX::SimpleMath::Vector3& size)
{
	std::shared_ptr<ColliderObject> createdEnemy = nullptr;

	// タイプ別のインスタンス生成
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

	// 共通プロパティ
	if (createdEnemy)
	{
		// FIXME: 全敵クラスの基底にSetMapが存在しないため、ダウンキャストで対応
		// 共通インターフェース(IEnemyなど)を用意できればこの分岐は削除可能
		if (auto e = std::dynamic_pointer_cast<Enemy>(createdEnemy)) e->SetMap(this);
		else if (auto s = std::dynamic_pointer_cast<StalkerEnemy>(createdEnemy)) s->SetMap(this);
		else if (auto l = std::dynamic_pointer_cast<loversEnemy>(createdEnemy)) l->SetMap(this);
	}

	return createdEnemy;
}



// ----------------------------------------------------------------
// ゴール鍵を生成し、対象の敵に追従設定を行う
// ----------------------------------------------------------------
void MakeMap::CreateKey(SceneBase* scene, std::shared_ptr<ColliderObject> targetEnemy, EnemyType type)
{
	if (!targetEnemy) return;

	// カギのサイズは固定（必要に応じて定数化推奨）
	const DirectX::SimpleMath::Vector3 keySize(20.0f, 20.0f, 20.0f);

	auto weakKey = scene->AddObject<GoalKey>(targetEnemy->GetPosition(), keySize);
	if (auto key = weakKey.lock())
	{
		key->SetTarget(targetEnemy); // 生成直後にターゲットへアタッチ

#ifdef _DEBUG
		// 敵タイプを文字列に変換
		const char* typeName = "Unknown";
		switch (type)
		{
		case EnemyType::Normal:  typeName = "Normal (通常敵)"; break;
		case EnemyType::Stalker: typeName = "Stalker (ストーカー)"; break;
		case EnemyType::Lover:   typeName = "Lover (恋人)"; break;
		}

		std::cout << "==========================================" << std::endl;
		std::cout << "[MakeMap] Key Attached info:" << std::endl;
		std::cout << " - Target Type : " << typeName << std::endl;
		std::cout << " - Position    : ("
			<< targetEnemy->GetPosition().x << ", "
			<< targetEnemy->GetPosition().z << ")" << std::endl;
		std::cout << "==========================================" << std::endl;
#endif
	}
}




// 敵配置セルのスコアを計算
float MakeMap::CalcEnemyScore(int x, int y) const
{
	// スタート・ゴールからの距離を計算
	int distFromPlayer = std::abs(StartX - x) + std::abs(StartY - y);
	int distFromGoal = std::abs(GoalX - x) + std::abs(GoalY - y);

	// ゴール距離に重みを掛けて調整
	float adjustedGoalDist = static_cast<float>(distFromGoal) * MAP::Config::EnemyGoalDistWeight;

	return std::min(static_cast<float>(distFromPlayer), adjustedGoalDist);
}



// 指定セルが通路かどうかを返す
bool MakeMap::IsWalkable(int x, int y) const
{
	if (x < 0 || y < 0 || x >= MAP::Config::MaxX || y >= MAP::Config::MaxY) return false;
	return (MAP[x][y] == 0);
}



// 指定セルのクリアランス値を返す
float MakeMap::GetClearance(int x, int y) const
{
	if (x < 0 || y < 0 || x >= MAP::Config::MaxX || y >= MAP::Config::MaxY) return 0.0f;
	return m_ClearanceMap[x][y];
}