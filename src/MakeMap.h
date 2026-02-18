#pragma once
#include"ColliderObject.h"
#include"SceneBase.h"
#include"Pathfinder.h"

class Block;

//生成した迷路を２次元配列に格納

namespace MAP {

	struct Config
	{
		//マップ関連
		static constexpr std::uint16_t MaxX = 31;//横セル数（X方向）必ず奇数にすること
		static constexpr std::uint16_t MaxY = 31;//縦セル数（Y方向）
		static constexpr float  BLOCK_SIZE = 55.0f; //１セルあたりの物理サイズ
		static constexpr float CornerRate = 75.0f;//曲がり角を作る確率(%)

		//敵関連
		static constexpr float EnemyGoalDistWeight = 0.7f;//敵のスタート位置をゴールから遠ざける重み
		static constexpr float EnemyDensity = 1.0f / 80.0f;//敵の密度（1セルあたりの敵の数）
		static constexpr int   MinEnemies = 2;//敵の数の最低値
		static constexpr int   MaxEnemies = 15;//敵の数の上限値
		static constexpr int   EnemyMinSeparation = 4;//敵同士の最低距離（マンハッタン距離）
		static constexpr int   EnemySpawnSafeRadius = 3; // 敵がスポーンしない安全圏（スタート/ゴールからの距離）

		//透明化アイテム関連
		static constexpr int   Num_InvisibleItem = 5; //配置する透明化アイテムの数

		//広場関連
		static constexpr int   SquareSize = 6; //広場の半径をマップ幅の何分の１にするか(セル単位)
		static constexpr int ClearanceSearchRadius = 15;// クリアランス計算で探索する最大半径

		// ゴール距離による壁色変化
		static constexpr int GoalNearRadius = 10;  // 赤ゾーン半径（マンハッタン距離）
		static constexpr int GoalMidRadius  = 20;  // 青ゾーン半径
	};
};


// ブロックの種類を定義
enum class BlockType 
{
	Normal,    // デフォルト（赤：ゴール近距離）
	MidGoal,   // ゴール中距離（青）
	FarGoal    // ゴール遠距離（緑）
};


struct ObjectSpec
{
	int gridX, gridY; // 迷路のグリッド座標
	DirectX::SimpleMath::Vector3 pos;
	DirectX::SimpleMath::Vector3 size;
	BlockType type = BlockType::Normal; //ブロックの種類。デフォルトは通常
};


struct MapBuildPlan 
{
	int map[MAP::Config::MaxX][MAP::Config::MaxY]{};
	std::vector<ObjectSpec> player;
	std::vector<ObjectSpec> enemy;
	std::vector<ObjectSpec> start;
	std::vector<ObjectSpec> floorBlocks;
	std::vector<ObjectSpec> blocks; // 置くべきブロックの一覧
	std::vector<ObjectSpec> goal;
	std::vector<ObjectSpec> InvisibleItem;//透明化アイテム用リスト


	int keyEnemyIndex = -1; // カギを持っている敵のインデックス
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



	//敵の種類
	enum class EnemyType
	{
		Normal,
		Stalker,
		Lover
	};



	// 迷路盤面の座標は構造体で管理
	struct MakeNowPosition {

		std::uint16_t x{};
		std::uint16_t y{};
	};

	// 迷路盤(基礎的なマップ生成に使う一時変数)
	int MAP[MAP::Config::MaxX][MAP::Config::MaxY]{}; // 1:壁, 0:通路
	int NowX = 0, NowY = 0;                                //計算の際の一時的な現在のポジション
	int StartX = 0, StartY = 0, GoalX = 0, GoalY = 0;              //スタートとゴールの座標
	std::uint16_t PlayerPosX = 0, PlayerPosY = 0;          //プレイヤーの座標
	int EnemyStartX = 0, EnemyStartY = 0;                  //敵のスタート座標
	direction dir;                                 //前回の進んだ方向
	std::uint16_t wallCount = 0;                   // 壁の数をカウントする変数
	std::vector<std::weak_ptr<Block>> blocks;                    // 複数のブロックを格納するベクター

	float m_ClearanceMap[MAP::Config::MaxX][MAP::Config::MaxY]; //最も近い壁までの距離を格納する配列

	

	//エリア管理用構造体
	struct CircleArea
	{
		int cx = -1;
		int cy = -1;
		int radius = 0;
	};

	CircleArea plazaInfo; // 広場の情報を保持


	MapBuildPlan plan;//マップ生成計画を格納する構造体

	// 乱数生成器とディストリビューション
	mutable std::random_device rd;
	mutable std::mt19937 gen;
	mutable std::uniform_int_distribution<int> dir_dist;
	mutable std::uniform_int_distribution<int> pos_x_dist;
	mutable std::uniform_int_distribution<int> pos_y_dist;
	mutable std::uniform_int_distribution<int> percent_dist;

	void Init();
	void SetWall(); //壁を配置
	void SetFloor(); //床を配置
	void SetObjects();//スタートとゴールの座標を設定
	void SettingBlocks();//ブロックを配置
	void SetLandmarks();//ランドマークを配置
	void SetPark();//広場を配置
	void SetItems();//透明化アイテムを配置

	direction Decide_Direction()const;//上下左右どちらに壁を作るか決める
	void WallCompleteCheck();//上下左右が全て作成中の壁ではないか確認
	void ResizeBlocks(const std::uint16_t);//ブロックの数に合わせてベクターのサイズを変更

	// 敵配置関連
	int CalcEnemyCountFromMap() const;// 敵の密度から敵の数を計算
	std::vector<GridCoord> CollectEnemySpawnCells() const;// 敵のスポーン可能セルを収集
	float CalcEnemyScore(int x, int y) const;// 敵スポーンセルのスコアを計算

	// 敵の種類を決定
	EnemyType DecideEnemyType(int index, int stalkerIdx, int loverIdx1, int loverIdx2) const;

	// 敵オブジェクトを生成
	std::shared_ptr<ColliderObject> CreateEnemy(SceneBase* scene, EnemyType type, const DirectX::SimpleMath::Vector3& pos, const DirectX::SimpleMath::Vector3& size);

	// カギを生成して敵に持たせる
	void CreateKey(SceneBase* scene, std::shared_ptr<ColliderObject> targetEnemy, EnemyType type);

public:

	MakeMap();
	~MakeMap() = default;

	// コピー・ムーブを禁止
	MakeMap(const MakeMap&) = delete;
	MakeMap& operator=(const MakeMap&) = delete;
	MakeMap(MakeMap&&) = delete;
	MakeMap& operator=(MakeMap&&) = delete;

	void CreatePlan();//各オブジェクトの配置情報を作成
	void SpawnObjects(SceneBase* scene);//マップオブジェクトを生成してシーンに追加

	const std::vector<std::weak_ptr<Block>>& GetBlocks() const { return blocks; }//全ブロックを取得

	size_t GetBlockCount() const { return blocks.size(); }//ブロックの総数を取得

	// スタートとゴールの座標を取得
	void GetStartGoal(int& startX, int& startY, int& goalX, int& goalY) const
	{
		startX = StartX; startY = StartY; goalX = GoalX; goalY = GoalY;
	}

	// 指定セルのクリアランスを取得
	float GetClearance(int gridX, int gridY) const;

	// クリアランス計算用関数の宣言
	void CalculateClearance();

	// 指定セルが壁でなければtrueを返す
	bool IsWalkable(int x, int y) const;

	// 内部の MAP 配列を取得（外部は読み取り専用）
	const int(&GetMap() const)[MAP::Config::MaxX][MAP::Config::MaxY]{ return MAP; }
	
	void DestroyWall(int x, int y);// 指定したグリッド座標の壁を通路に変更する

};

