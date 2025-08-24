#pragma once
#include <vector>
#include"ColliderObject.h"
#include"Block.h"

#include <random>

//マップは壁のばし法を用いて生成した迷路を２次元配列に格納

namespace MAP {

	struct Config {
		static constexpr std::uint16_t MaxX = 11;//横セル数（X方向）必ず奇数にすること
		static constexpr std::uint16_t MaxY = 11;//縦セル数（Y方向）
		static constexpr float  BLOCK_SIZE = 55.0f; //１セルあたりの物理サイズ
	};
};

class MakeMap
{
private:

	//壁を作る方向
	enum direction
	{
		Up,
		Down,
		Left,
		Right
	};

	// 迷路盤面の座標は構造体で管理
	struct MakeNowPosition {

		std::uint16_t x;
		std::uint16_t y;
	};

	// 迷路盤
	int MAP[MAP::Config::MaxX][MAP::Config::MaxY]; // 1:壁, 0:通路
	int NowX, NowY;                                //計算の際の一時的な現在のポジション
	int StartX, StartY, GoalX, GoalY;              //スタートとゴールの座標
	std::uint16_t PlayerPosX, PlayerPosY;          //プレイヤーの座標
	int EnemyStartX, EnemyStartY;                  //敵のスタート座標
	direction dir;                                 //前回の進んだ方向
	std::uint16_t wallCount = 0;                   // 壁の数をカウントする変数
	std::vector<std::weak_ptr<Block>> blocks;                    // 複数のブロックを格納するベクター

	DirectX::SimpleMath::Vector3 GoleSize; //ゴールのサイズ

	// 乱数生成器とディストリビューション
	mutable std::random_device rd;
	mutable std::mt19937 gen;
	mutable std::uniform_int_distribution<int> dir_dist;
	mutable std::uniform_int_distribution<int> pos_x_dist;
	mutable std::uniform_int_distribution<int> pos_y_dist;

	void Init();
	void MakeWall(); //壁を生成
	void Set_Start_Goal(); //スタートとゴールの座標を設定
	direction Decide_Direction()const;//上下左右どちらに壁を作るか決める
	void WallCompleteCheck();//上下左右が全て作成中の壁ではないか確認
	void ResizeBlocks(const std::uint16_t);//ブロックを作成
	void SettingBlocks();//ブロックを配置

public:

	MakeMap();
	~MakeMap() = default;

	// コピー・ムーブを禁止
	MakeMap(const MakeMap&) = delete;
	MakeMap& operator=(const MakeMap&) = delete;
	MakeMap(MakeMap&&) = delete;
	MakeMap& operator=(MakeMap&&) = delete;

	void Create();//マップを生成

	const std::vector<std::weak_ptr<Block>>& GetBlocks() const { return blocks; }//全ブロックを取得

	size_t GetBlockCount() const { return blocks.size(); }//ブロックの総数を取得

	// スタートとゴールの座標を取得
	void GetStartGoal(int& startX, int& startY, int& goalX, int& goalY) const
	{
		startX = StartX; startY = StartY; goalX = GoalX; goalY = GoalY;
	}

	void SpawnEnemy();

	// 指定セルが壁でなければtrueを返す
	bool IsWalkable(int x, int y) const {
		if (x < 0 || y < 0 || x >= MAP::Config::MaxX || y >= MAP::Config::MaxY) return false;
		return MAP[x][y] == 0;
	}

	// 内部の MAP 配列を取得（外部は読み取り専用）
	const int(&GetMap() const)[MAP::Config::MaxX][MAP::Config::MaxY]{ return MAP; }
};

