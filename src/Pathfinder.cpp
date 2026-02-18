#include "pch.h"
#include "Pathfinder.h"
#include "MakeMap.h"


using DirectX::SimpleMath::Vector3;

// A*探索用のノード
struct Node
{
	GridCoord coord;
	float gCost;
	float hCost;
	float fCost() const { return gCost + hCost; }
	GridCoord parent;
};

// 優先度キューの順序付けに必要
struct NodeCompare
{
	bool operator()(const Node& a, const Node& b) const
	{
		return a.fCost() > b.fCost();
	}
};

std::vector<GridCoord> Pathfinder::FindPath(const MakeMap* map,
	const GridCoord& start,
	const GridCoord& goal)
{
	// スタートとゴールが同じ場合はリターン
	if (start.x == goal.x && start.y == goal.y)
	{
		return { start };
	}

	// MakeMapを介して歩行可能性をテストするラムダを定義
	auto isWalkable = [map](int x, int y) -> bool
		{
			return map->IsWalkable(x, y);
		};

	const int maxX = MAP::Config::MaxX;
	const int maxY = MAP::Config::MaxY;


	// グリッド座標を一意のキーに変換するラムダ
	auto flatten = [maxY](int x, int y) { return x * maxY + y; };
	std::unordered_map<int, Node> cameFrom;

	std::priority_queue<Node, std::vector<Node>, NodeCompare> open;
	Node startNode{ start, 0.0f, Heuristic(start, goal), { -1, -1 } };
	cameFrom.emplace(flatten(start.x, start.y), startNode);
	open.push(startNode);

	// 4方向接続の隣接オフセット
	static const GridCoord neighbours[4] = { {1,0}, {-1,0}, {0,1}, {0,-1} };

	while (!open.empty())
	{
		Node current = open.top();
		open.pop();

		// ゴールに到達した場合、パスを再構築
		if (current.coord.x == goal.x && current.coord.y == goal.y)
		{
			std::vector<GridCoord> path;
			path.push_back(current.coord);
			GridCoord back = current.parent;

			// 無効な親に到達するまで親を辿る
			while (back.x >= 0)
			{
				auto it = cameFrom.find(flatten(back.x, back.y));
				if (it == cameFrom.end()) break;

				path.push_back(it->second.coord);
				back = it->second.parent;
			}

			// スタートからゴール順にするため反転
			std::reverse(path.begin(), path.end());
			return path;
		}

		// 隣接セルを探索
		for (const auto& offset : neighbours)
		{
			int nx = current.coord.x + offset.x;
			int ny = current.coord.y + offset.y;
			if (nx < 0 || ny < 0 || nx >= maxX || ny >= maxY) continue;
			if (!isWalkable(nx, ny)) continue;

			float tentativeG = current.gCost + 1.0f; // セル間の均一コスト
			int key = flatten(nx, ny);
			auto it = cameFrom.find(key);
			if (it != cameFrom.end() && tentativeG >= it->second.gCost)
			{
				continue;
			}

			Node neighbourNode;
			neighbourNode.coord = { nx, ny };
			neighbourNode.gCost = tentativeG;
			neighbourNode.hCost = Heuristic(neighbourNode.coord, goal);
			neighbourNode.parent = current.coord;
			cameFrom[key] = neighbourNode;
			open.push(neighbourNode);
		}
	}

	// パスが見つからない
	return {};
}




//コストを考慮するバージョン
std::vector<GridCoord> Pathfinder::FindPathWithWeights(const MakeMap* map,
	const GridCoord& start,
	const GridCoord& goal,
	const std::unordered_map<int, float>& costMap)
{
	// スタートとゴールが同じ場合はリターン
	if (start.x == goal.x && start.y == goal.y)
	{
		return { start };
	}

	// MakeMapを介して歩行可能性をテストするラムダを定義
	auto isWalkable = [map](int x, int y) -> bool
		{
			return map->IsWalkable(x, y);
		};

	const int maxX = MAP::Config::MaxX;
	const int maxY = MAP::Config::MaxY;


	// グリッド座標を一意のキーに変換するラムダ
	auto flatten = [maxY](int x, int y) { return x * maxY + y; };
	std::unordered_map<int, Node> cameFrom;

	std::priority_queue<Node, std::vector<Node>, NodeCompare> open;
	Node startNode{ start, 0.0f, Heuristic(start, goal), { -1, -1 } };
	cameFrom.emplace(flatten(start.x, start.y), startNode);
	open.push(startNode);

	// 4方向接続の隣接オフセット
	static const GridCoord neighbours[4] = { {1,0}, {-1,0}, {0,1}, {0,-1} };

	while (!open.empty())
	{
		Node current = open.top();
		open.pop();

		// ゴールに到達した場合、パスを再構築
		if (current.coord.x == goal.x && current.coord.y == goal.y)
		{
			std::vector<GridCoord> path;
			path.push_back(current.coord);
			GridCoord back = current.parent;

			// 無効な親に到達するまで親を辿る
			while (back.x >= 0)
			{
				auto it = cameFrom.find(flatten(back.x, back.y));
				if (it == cameFrom.end()) break;

				path.push_back(it->second.coord);
				back = it->second.parent;
			}

			// スタートからゴール順にするため反転
			std::reverse(path.begin(), path.end());
			return path;
		}

		// 隣接セルを探索
		for (const auto& offset : neighbours)
		{
			int nx = current.coord.x + offset.x;
			int ny = current.coord.y + offset.y;
			if (nx < 0 || ny < 0 || nx >= maxX || ny >= maxY) continue;
			if (!isWalkable(nx, ny)) continue;

			float moveCost = 1.0f; // 基本コスト

			// もしコストマップにこの座標(nx, ny)が含まれていたら、コストを加算
			int nextKey = flatten(nx, ny);
			if (costMap.find(nextKey) != costMap.end())
			{
				moveCost += costMap.at(nextKey);
			}

			float tentativeG = current.gCost + moveCost;

			auto it = cameFrom.find(nextKey);
			if (it != cameFrom.end() && tentativeG >= it->second.gCost)
			{
				continue;
			}

			Node neighbourNode;
			neighbourNode.coord = { nx, ny };
			neighbourNode.gCost = tentativeG;
			neighbourNode.hCost = Heuristic(neighbourNode.coord, goal);
			neighbourNode.parent = current.coord;
			cameFrom[nextKey] = neighbourNode;
			open.push(neighbourNode);
		}
	}

	// パスが見つからない
	return {};
}





Vector3 Pathfinder::GridToWorld(const MakeMap* map, const GridCoord& coord)
{
	
	const float HALF_BLOCK = MAP::Config::BLOCK_SIZE / 2.0f;
	const float MAP_CENTER_X = MAP::Config::MaxX / 2.0f * MAP::Config::BLOCK_SIZE;
	const float MAP_CENTER_Y = MAP::Config::MaxY / 2.0f * MAP::Config::BLOCK_SIZE;

	float worldX = 0.0f + (HALF_BLOCK + MAP_CENTER_X - (MAP::Config::BLOCK_SIZE * coord.x));
	float worldY = 0.0f - (MAP::Config::BLOCK_SIZE / 2.0f);
	float worldZ = 0.0f - (HALF_BLOCK - MAP_CENTER_Y + (MAP::Config::BLOCK_SIZE * coord.y));

	return Vector3(worldX, 0.0f, worldZ);
}

GridCoord Pathfinder::WorldToGrid(const MakeMap* map, const Vector3& pos)
{
	
	const float HALF_BLOCK = MAP::Config::BLOCK_SIZE / 2.0f;
	const float MAP_CENTER_X = MAP::Config::MaxX / 2.0f * MAP::Config::BLOCK_SIZE;
	const float MAP_CENTER_Y = MAP::Config::MaxY / 2.0f * MAP::Config::BLOCK_SIZE;

	// X座標の逆算
	float gridX_f = ((HALF_BLOCK + MAP_CENTER_X) - pos.x) / MAP::Config::BLOCK_SIZE;

	// Z座標の逆算
	float gridY_f = ((-pos.z - HALF_BLOCK + MAP_CENTER_Y)) / MAP::Config::BLOCK_SIZE;

	int x = static_cast<int>(std::round(gridX_f));
	int y = static_cast<int>(std::round(gridY_f));

	// 境界内にクランプ
	x = std::clamp(x, 0, static_cast<int>(MAP::Config::MaxX) - 1);
	y = std::clamp(y, 0, static_cast<int>(MAP::Config::MaxY) - 1);

	return { x, y };
}

std::vector<Vector3> Pathfinder::SmoothPath(const std::vector<Vector3>& path,
	const std::function<bool(const Vector3&, const Vector3&)>& isWalkable)
{
	if (path.size() < 3)
	{
		return path;
	}

	std::vector<Vector3> result;
	size_t current = 0;
	result.push_back(path[current]);
	while (current < path.size() - 1)
	{
		// 直接線分が歩行可能な限り、可能な限り先を見ようと試みる
		size_t furthest = current + 1;
		for (size_t next = current + 2; next < path.size(); ++next)
		{
			if (isWalkable(path[current], path[next]))
			{
				furthest = next;
			}
			else
			{
				break;
			}
		}
		current = furthest;
		result.push_back(path[current]);
	}
	return result;
}