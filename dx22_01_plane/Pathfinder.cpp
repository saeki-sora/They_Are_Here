#include "Pathfinder.h"
#include "MakeMap.h"
#include <queue>
#include <unordered_map>
#include <functional>

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
	// スタートとゴールが同じ場合は早期リターン
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

	// 平坦化座標をキーとするunordered_mapを使用して、発見された最低gCostを追跡
	// 平坦化により、セルごとに一意のキーが保証される
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
			// スタート→ゴール順にするため反転
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
				continue; // より良いパスではない
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

Vector3 Pathfinder::GridToWorld(const MakeMap* map, const GridCoord& coord)
{
	// グリッド(0,0)が負の半範囲に整列するようにオフセットを計算
	float halfX = static_cast<float>(MAP::Config::MaxX) * MAP::Config::BLOCK_SIZE * 0.5f;
	float halfY = static_cast<float>(MAP::Config::MaxY) * MAP::Config::BLOCK_SIZE * 0.5f;
	float worldX = coord.x * MAP::Config::BLOCK_SIZE - halfX + MAP::Config::BLOCK_SIZE * 0.5f;
	float worldZ = coord.y * MAP::Config::BLOCK_SIZE - halfY + MAP::Config::BLOCK_SIZE * 0.5f;
	return Vector3(worldX, 0.0f, worldZ);
}

GridCoord Pathfinder::WorldToGrid(const MakeMap* map, const Vector3& pos)
{
	float halfX = static_cast<float>(MAP::Config::MaxX) * MAP::Config::BLOCK_SIZE * 0.5f;
	float halfY = static_cast<float>(MAP::Config::MaxY) * MAP::Config::BLOCK_SIZE * 0.5f;
	int x = static_cast<int>((pos.x + halfX) / MAP::Config::BLOCK_SIZE);
	int y = static_cast<int>((pos.z + halfY) / MAP::Config::BLOCK_SIZE);
	// 境界内にクランプ
	x = std::clamp(x, 0, static_cast<int>(MAP::Config::MaxX) - 1);
	y = std::clamp(y, 0, static_cast<int>(MAP::Config::MaxY) - 1);
	return { x, y };
}

std::vector<Vector3> Pathfinder::SmoothPath(
	const std::vector<Vector3>& path,
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