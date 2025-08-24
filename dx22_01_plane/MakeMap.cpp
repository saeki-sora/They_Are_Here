#include "MakeMap.h"
#include "ColliderObject.h"
#include"Game.h"
#include"Block.h"
#include"Pole.h"
#include "Player.h"
#include <cassert>
#include "Enemy.h"


MakeMap::MakeMap()
	: gen(rd())  //シードを初期化
	, dir_dist(0, 3)  //方向用
	, pos_x_dist(0, MAP::Config::MaxX - 1)
	, pos_y_dist(0, MAP::Config::MaxY - 1)
{}

void MakeMap::Create()
{
	Init();//マップ全体を初期化
	MakeWall();//壁を生成
	ResizeBlocks(wallCount); // 壁の数に応じてブロックをリサイズ
	SettingBlocks(); // ブロックを配置
	Set_Start_Goal();//スタートとゴールの座標を設定

	//デバッグ用で出来た迷路をcoutで表示
	for (int x = 0; x < MAP::Config::MaxX; ++x) {

		for (int y = 0; y < MAP::Config::MaxY; ++y) {

			std::cout << MAP[x][y] << " ";
		}
		std::cout << std::endl;
	}

	//見やすい版
	std::cout << "\n[Pretty Maze]\n";
	for (int x = 0; x < MAP::Config::MaxX; ++x) {
		for (int y = 0; y < MAP::Config::MaxY; ++y) {
			std::cout << (MAP[x][y] == 1 ? "##" : "  ");
		}
		std::cout << '\n';
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

	blocks.clear();
	wallCount = 0;
}


void MakeMap::MakeWall()
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
		int attempts = 0;

		//未探索の柱を探す
		for (int x = 0; x <= MAP::Config::MaxX - 1; ++x) {

			for (int y = 0; y <= MAP::Config::MaxY - 1; ++y) {

				if (MAP[x][y] == 2)
				{
					NowX = x;
					NowY = y;
					found = true; // フラグをセット
					break;
				}

				//未探索の柱が無くなったら終了
				if (x == MAP::Config::MaxX - 1 && y == MAP::Config::MaxY - 1) {

					end = true;
				}
			}

			if (found)break;//未探索の柱が見つかれば外側のループを抜ける
		}

		//未探索の柱が見つからなかったら終了
		if (!found) {
			end = true;
		}

		while (MAP[NowX][NowY] != 1 && attempts < MAX_ATTEMPTS)
		{
			//上下左右どちらに壁を作るか決める
			dir = Decide_Direction();

			switch (dir) {

			case Up://上に壁を作る

				//その方向に壁を作ることができるか判定(作成中の壁に当たるなら作成できない)
				if (NowX >= 2 && NowX <= MAP::Config::MaxX - 3 && MAP[NowX - 2][NowY] != 3) {

					//NowXをNowX-2まで減らしながら、その道中に3を代入する
					for (int i = NowX; i >= NowX - 2; --i) {

						//現在地が壁なら壁を作成しない
						if (MAP[i][NowY] == 1) break;

						else {
							MAP[i][NowY] = 3;
						}
					}
					//現在地を更新
					NowX = NowX - 2;
				}
				break;

			case Down:

				if (NowX <= MAP::Config::MaxX - 3 && NowX >= 2 && MAP[NowX + 2][NowY] != 3) {

					for (int i = NowX; i <= NowX + 2 && i < MAP::Config::MaxX - 1; ++i) {

						if (MAP[i][NowY] == 1) break;

						else {
							MAP[i][NowY] = 3;
						}
					}
					NowX = NowX + 2;
				}
				break;

			case Left:

				if (NowY >= 2 && NowY <= MAP::Config::MaxY - 3 && MAP[NowX][NowY - 2] != 3) {

					for (int i = NowY; i >= NowY - 2 && i > 0; --i) {

						if (MAP[NowX][i] == 1) break;

						else {
							MAP[NowX][i] = 3;
						}
					}
					NowY = NowY - 2;
				}
				break;

			case Right:

				if (NowY <= MAP::Config::MaxY - 3 && NowY >= 2 && MAP[NowX][NowY + 2] != 3) {

					for (int i = NowY; i <= NowY + 2; ++i) {

						if (MAP[NowX][i] == 1) break;

						else {
							MAP[NowX][i] = 3;
						}
					}
					NowY = NowY + 2;
				}
				break;
			}

			attempts++;

			//試行回数が上限に達したらエラー(デバッグビルドでのみ動作)
			assert(attempts < MAX_ATTEMPTS && "MakeMap.cpp: Wall extension failed - too many attempts");

			//上下左右が全て作成中の壁ではないか確認
			WallCompleteCheck();
		}

		//作成中の壁を壁に変更
		for (int x = 0; x < MAP::Config::MaxX; ++x) {

			for (int y = 0; y < MAP::Config::MaxY; ++y) {

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
		for (int x = 0; x < MAP::Config::MaxX; ++x) {

			for (int y = 0; y < MAP::Config::MaxY; ++y) {

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
void MakeMap::Set_Start_Goal() {

	// ループの上限を設定して無限ループを防止
	int max_attempts = 100;
	int attempts = 0;

	//MAPの中の0のマスからランダムに一つ選ぶ
	while (attempts < max_attempts) {
		attempts++;

		int x = pos_x_dist(gen);
		int y = pos_y_dist(gen);

		if (MAP[x][y] == 0) {

			//選んだ0のマスから最も離れている0のマスを二つ選ぶ
			int max_distance = 0;
			int max_x = x;
			int max_y = y;

			for (int i = 0; i < MAP::Config::MaxX; ++i) {
				for (int j = 0; j < MAP::Config::MaxY; ++j) {
					if (MAP[i][j] == 0) {
						int distance = abs(x - i) + abs(y - j); // マンハッタン距離を計算

						// 距離が最大のマスを更新
						if (distance > max_distance) {
							max_distance = distance;
							max_x = i;
							max_y = j;
						}
					}
				}
			}

			// スタートとゴールが異なる座標かを確認
			if (x != max_x || y != max_y) 
			{
				// スタートとゴールを設定
				StartX = x;
				StartY = y;
				GoalX = max_x;
				GoalY = max_y;

				// 敵のスタート位置を設定
				int max_distance_e = 0;
				// スタート地点から最も遠い通路を探す
				for (int i = 0; i < MAP::Config::MaxX; ++i) {
					for (int j = 0; j < MAP::Config::MaxY; ++j) {
						if (MAP[i][j] == 0 && (i != GoalX || j != GoalY))// ゴール地点以外
						{ 
							int distance = abs(StartX - i) + abs(StartY - j);
							if (distance > max_distance_e) {
								max_distance_e = distance;
								EnemyStartX = i;
								EnemyStartY = j;
							}
						}
					}
				}

				//全員の生成処理
				const float HALF_BLOCK = MAP::Config::BLOCK_SIZE / 2.0f;
				const float MAP_CENTER_X = MAP::Config::MaxX / 2.0f * MAP::Config::BLOCK_SIZE;
				const float MAP_CENTER_Y = MAP::Config::MaxY / 2.0f * MAP::Config::BLOCK_SIZE;

				// プレイヤー生成
				float p_posX = 0.0f + (HALF_BLOCK + MAP_CENTER_X - (MAP::Config::BLOCK_SIZE * StartX));
				float p_posZ = 0.0f - (HALF_BLOCK - MAP_CENTER_Y + (MAP::Config::BLOCK_SIZE * StartY));
				Game::GetInstance().AddObject<Player>(Vector3(p_posX, 0.0f, p_posZ), Vector3(1.0f, 0.3f, 1.0f));

				// ゴール生成
				float g_posX = 0.0f + (HALF_BLOCK + MAP_CENTER_X - (MAP::Config::BLOCK_SIZE * GoalX));
				float g_posZ = 0.0f - (HALF_BLOCK - MAP_CENTER_Y + (MAP::Config::BLOCK_SIZE * GoalY));
				GoleSize = { MAP::Config::BLOCK_SIZE / 2.0f , MAP::Config::BLOCK_SIZE * 2.0f, MAP::Config::BLOCK_SIZE / 2.0f };
				Game::GetInstance().AddObject<Pole>(Vector3(g_posX, 0.0f, g_posZ), GoleSize);

				// 敵生成
				float e_posX = 0.0f + (HALF_BLOCK + MAP_CENTER_X - (MAP::Config::BLOCK_SIZE * EnemyStartX));
				float e_posZ = 0.0f - (HALF_BLOCK - MAP_CENTER_Y + (MAP::Config::BLOCK_SIZE * EnemyStartY));
				auto enemy_weak = Game::GetInstance().AddObject<Enemy>(Vector3(e_posX, 0.0f, e_posZ), Vector3(5.0f, 1.0f, 5.0f));
				if (auto enemy_shared = enemy_weak.lock())
				{
					// 生成した敵に、このMakeMapインスタンス自身へのポインタを渡す
					enemy_shared->SetMap(this);
				}

				break;// 全員配置完了したらループを抜ける
			}
		}
	}

	// スタートとゴールが設定できなかった場合のフォールバック処理
	assert(attempts < max_attempts && "MakeMap.cpp: Failed to assign start and goal positions");
}


void MakeMap::ResizeBlocks(const std::uint16_t count)
{
	blocks.clear();
	blocks.shrink_to_fit();
	blocks.reserve(count);// 壁の数を計算してメモリを事前確保
}
	
void MakeMap::SettingBlocks()
{
	const float HALF_BLOCK = MAP::Config::BLOCK_SIZE / 2.0f;// ブロックの半分のサイズを計算
	const float MAP_CENTER_X = MAP::Config::MaxX / 2.0f * MAP::Config::BLOCK_SIZE;// マップの中心座標を計算
	const float MAP_CENTER_Y = MAP::Config::MaxY / 2.0f * MAP::Config::BLOCK_SIZE;
	DirectX::SimpleMath::Vector3 blockSize(HALF_BLOCK, MAP::Config::BLOCK_SIZE, HALF_BLOCK);// ブロックのサイズを設定

	for (int x = 0; x < MAP::Config::MaxX; ++x) {

		for (int y = 0; y < MAP::Config::MaxY; ++y) {

			// スタート・ゴール・空白はスキップ
			if ((x == StartX && y == StartY) ||
				(x == GoalX && y == GoalY) ||
				(MAP[x][y] == 0))
			{
				continue;
			}

			// 3D空間での位置計算
			float posX = 0.0f + (HALF_BLOCK + MAP_CENTER_X - (MAP::Config::BLOCK_SIZE * x));
			float posY = 0.0f;
			float posZ = 0.0f - (HALF_BLOCK - MAP_CENTER_Y + (MAP::Config::BLOCK_SIZE * y));

			DirectX::SimpleMath::Vector3 position(posX, posY, posZ);

			blocks.emplace_back(Game::GetInstance().AddObject<Block>(position, blockSize));

		}
	}
}