#pragma once


using DirectX::SimpleMath::Vector3;

class MakeMap;

struct GridCoord
{
    int x;
    int y;
};

// MakeMap内のグリッドで動作するシンプルなA*ベースの経路探索クラス
class Pathfinder
{
public:

    //グリッド座標の最短経路を返す
    static std::vector<GridCoord> FindPath(const MakeMap* map,
        const GridCoord& start,
        const GridCoord& goal);

    //コストを考慮するバージョン
    static std::vector<GridCoord> FindPathWithWeights(const MakeMap* map,
        const GridCoord& start,
        const GridCoord& goal,
        const std::unordered_map<int, float>& costMap);

    // グリッド座標をワールド座標への変換
    static Vector3 GridToWorld(const MakeMap* map,
        const GridCoord& coord);

    // ワールド座標をグリッド座標への変換
    static GridCoord WorldToGrid(const MakeMap* map,const Vector3& pos);

    static std::vector<Vector3> SmoothPath(
        const std::vector<Vector3>& path,
        const std::function<bool(const Vector3&, const Vector3&)>& isWalkable);

private:

    // A*ヒューリスティック用のヘルパー（マンハッタン距離）
    static float Heuristic(const GridCoord& a, const GridCoord& b)
    {
        return static_cast<float>(std::abs(a.x - b.x) + std::abs(a.y - b.y));
    }
};