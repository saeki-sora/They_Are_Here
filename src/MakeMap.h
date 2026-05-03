#pragma once
#include "ColliderObject.h"
#include "SceneBase.h"
#include "Pathfinder.h"
#include "DifficultyConfig.h"

class Block;


//マップ全体で共有される「コンパイル時定数」のみを置く
namespace MAP
{
	struct Config
	{
		// ---- マップ配列の上限サイズ----
		// Hard難易度の 41x41 が最大。これ以上の難易度を追加する場合は拡張する。
		static constexpr std::uint16_t MAX_SIZE = 41;

		// ---- 変更しない定数 ----
		static constexpr float  BLOCK_SIZE = 55.0f;       // 1セルの物理サイズ
		static constexpr float  CornerRate = 75.0f;       // 曲がり角確率(%)

		static constexpr float  EnemyGoalDistWeight = 0.7f;  // 敵スポーン：ゴール遠ざけ重み
		static constexpr float  EnemyDensity = 1.0f / 80.f; // 敵密度（敵数自動計算時）
		static constexpr int    MinEnemies = 2;           // 敵の最低数（自動計算時）
		static constexpr int    MaxEnemies = 15;          // 敵の最大数（自動計算時）
		static constexpr int    EnemyMinSeparation = 4;           // 敵同士の最低マンハッタン距離
		static constexpr int    EnemySpawnSafeRadius = 3;           // スタート/ゴール周辺の安全圏

		static constexpr int    SquareSize = 6;           // 広場半径（マップ幅の 1/N）
		static constexpr int    ClearanceSearchRadius = 15;          // クリアランス探索半径

		static constexpr float GOAL_NEAR_RATIO = 1.0f / 3.0f;//ゴール近距離ゾーンの比率（全難易度共通）
		static constexpr float GOAL_MID_RATIO = 2.0f / 3.0f;//ゴール中距離ゾーンの比率（全難易度共通）

		// ランプ
		static constexpr float LampRangeScale = 2.5f;   // 照射範囲 = BLOCK_SIZE × この値
		static constexpr float LampHeightRatio = 0.6f;   // Y座標  = BLOCK_SIZE × この値
		static constexpr float FlameColorG = 0.55f;
		static constexpr float FlameColorB = 0.1f;

		// コライダーサイズ
		static constexpr float EnemySize = 0.3f;
		static constexpr float EnemyFallbackSize = 0.7f; // フォールバック配置時のサイズ（通常と異なる。要確認）
		static constexpr float PlayerSizeXZ = 1.0f;
		static constexpr float PlayerSizeY = 0.3f;
		static constexpr float KeySize = 20.0f;
		static constexpr float FloorThickness = 10.0f;  // 床オブジェクトのY厚み
		static constexpr float ItemHeightRatio = 0.25f;  // アイテムのY座標 = BLOCK_SIZE × この値

		// 配置試行上限
		static constexpr int MaxPlaceAttempts = 100;
		static constexpr int MaxWallAttempts = 1000;
	};
}


// ---- ブロックの種類（ゴールからの距離で色が変わる） ----
enum class BlockType
{
	Normal,    // 赤：ゴール近距離
	MidGoal,   // 青：ゴール中距離
	FarGoal    // 緑：ゴール遠距離
};


// ---- 1オブジェクトの配置仕様 ----
struct ObjectSpec
{
	int gridX, gridY;
	DirectX::SimpleMath::Vector3 pos;
	DirectX::SimpleMath::Vector3 size;
	BlockType type = BlockType::Normal;
};


// ---- ランプ1個の配置仕様 ----
struct LampSpec
{
	DirectX::SimpleMath::Vector3 position; // 壁面から通路側にオフセットしたワールド座標
	DirectX::SimpleMath::Vector3 faceDir;  // 通路方向の単位ベクトル
};


// ---- マップ生成計画 ----
struct MapBuildPlan
{
	// 内部配列は MAX_SIZE で確保
	int map[MAP::Config::MAX_SIZE][MAP::Config::MAX_SIZE]{};

	std::vector<ObjectSpec> player;
	std::vector<ObjectSpec> enemy;
	std::vector<ObjectSpec> start;
	std::vector<ObjectSpec> floorBlocks;
	std::vector<ObjectSpec> blocks;
	std::vector<ObjectSpec> goal;
	std::vector<ObjectSpec> InvisibleItem;
	std::vector<LampSpec>   lamps;

	int keyEnemyIndex = -1; // カギを持つ敵のインデックス
};


// ================================================================================
// MakeMap クラス
// ================================================================================

class MakeMap
{
private:

	// ---- 壁を伸ばす方向 ----
	enum direction { Up, Down, Left, Right };

	// ---- 敵の種類 ----
	enum class EnemyType { Normal, Stalker, Lover };

	// ---- 迷路生成中の作業位置 ----
	struct MakeNowPosition
	{
		std::uint16_t x{};
		std::uint16_t y{};
	};

	// ================================================================================
	// 難易度によって変わる値
	// ================================================================================
	int m_SizeX = 31;  // 使用するマップのX幅（奇数）
	int m_SizeY = 31;  // 使用するマップのY幅（奇数）
	int m_NumInvisibleItems = 5;   // 配置するアイテム数
	int m_GoalNearRadius = 0;  // 赤ゾーン半径（Configure()で比率から計算）
	int m_GoalMidRadius = 0;  // 青ゾーン半径（Configure()で比率から計算）
	int m_EnemyCountOverride = 0; // 敵の数上書き設定（0なら自動計算）
	int m_StalkerCount = 0;     // Stalker 敵の数（Configure() でセット）
	int m_LoverPairCount = 0;     // Lovers ペア数（Configure() でセット）
	int m_LampCount = 16;          // マップに配置するランプの個数

	// ---- 迷路本体の2次元配列 ----
	int MAP[MAP::Config::MAX_SIZE][MAP::Config::MAX_SIZE]{};

	// ---- 生成中に使う一時変数 ----
	int NowX = 0, NowY = 0;
	int StartX = 0, StartY = 0, GoalX = 0, GoalY = 0;
	std::uint16_t PlayerPosX = 0, PlayerPosY = 0;
	int EnemyStartX = 0, EnemyStartY = 0;
	direction dir;
	std::uint16_t wallCount = 0;
	std::vector<std::weak_ptr<Block>> blocks;

	// ---- クリアランスマップ（各セルから最も近い壁までの距離） ----
	float m_ClearanceMap[MAP::Config::MAX_SIZE][MAP::Config::MAX_SIZE]{};

	// ---- 広場エリア情報 ----
	struct CircleArea { int cx = -1; int cy = -1; int radius = 0; };
	CircleArea plazaInfo;

	// ---- 生成計画（バックグラウンドで作成し SpawnObjects で使う） ----
	MapBuildPlan plan;

	// ---- 乱数 ----
	mutable std::random_device rd;
	mutable std::mt19937 gen;
	mutable std::uniform_int_distribution<int> dir_dist;
	mutable std::uniform_int_distribution<int> pos_x_dist;  // Configure() で再初期化
	mutable std::uniform_int_distribution<int> pos_y_dist;  // Configure() で再初期化
	mutable std::uniform_int_distribution<int> percent_dist;

	// ---- 生成サブ関数 ----
	void Init();
	void SetWall();
	void SetFloor();
	void SetObjects();
	void SettingBlocks();
	void SetLandmarks();
	void SetPark();
	void SetItems();
	void SetLamps();

	direction Decide_Direction() const;
	void WallCompleteCheck();
	void ResizeBlocks(const std::uint16_t);

	// ---- 敵配置 ----
	int CalcEnemyCountFromMap() const;
	std::vector<GridCoord> CollectEnemySpawnCells() const;
	float CalcEnemyScore(int x, int y) const;
	EnemyType DecideEnemyType(int index, const std::vector<int>& stalkerIndices, int loverIdx1, int loverIdx2) const;
	std::shared_ptr<ColliderObject> CreateEnemy(
		SceneBase* scene, EnemyType type,
		const DirectX::SimpleMath::Vector3& pos,
		const DirectX::SimpleMath::Vector3& size);
	void CreateKey(SceneBase* scene,
		std::shared_ptr<ColliderObject> targetEnemy, EnemyType type);

public:

	MakeMap();
	~MakeMap() = default;

	// コピー・ムーブ禁止
	MakeMap(const MakeMap&) = delete;
	MakeMap& operator=(const MakeMap&) = delete;
	MakeMap(MakeMap&&) = delete;
	MakeMap& operator=(MakeMap&&) = delete;

	// ---- 難易度設定を適用する ----
	void Configure(const DifficultyConfig& config);

	// ---- マッププラン生成 ----
	void CreatePlan();

	// ---- シーンへのオブジェクト配置 ----
	void SpawnObjects(SceneBase* scene);

	// ---- マップサイズ取得 ----
	int GetSizeX() const { return m_SizeX; }
	int GetSizeY() const { return m_SizeY; }
	int GetGoalNearRadius() const { return m_GoalNearRadius; }  // ゴール近接ゾーン半径（赤）
	int GetGoalMidRadius()  const { return m_GoalMidRadius; }  // ゴール中距離ゾーン半径（青）

	// ---- ブロック情報取得 ----
	const std::vector<std::weak_ptr<Block>>& GetBlocks() const { return blocks; }
	size_t GetBlockCount() const { return blocks.size(); }

	// ---- スタート・ゴール座標取得 ----
	void GetStartGoal(int& startX, int& startY, int& goalX, int& goalY) const { startX = StartX; startY = StartY; goalX = GoalX; goalY = GoalY; }

	// ---- クリアランス情報取得 ----
	float GetClearance(int gridX, int gridY) const;
	void  CalculateClearance();
	bool  IsWalkable(int x, int y) const;

	// GetMap(): 内部配列への読み取り専用参照
	const int(&GetMap() const)[MAP::Config::MAX_SIZE][MAP::Config::MAX_SIZE]
	{
		return MAP;
	}

	void DestroyWall(int x, int y);
};
