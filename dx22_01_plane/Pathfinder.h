#pragma once
#include <vector>
#include <SimpleMath.h>

using DirectX::SimpleMath::Vector3;

class MakeMap; // 重いヘッダーのインクルードを避けるため前方宣言

struct GridCoord
{
    int x;
    int y;
};

// MakeMap内のグリッドで動作するシンプルなA*ベースの経路探索クラス

// 経路探索は、マップの各セルを歩行可能（値 != 1）または歩行不可能（値 == 1）として扱い、
// マンハッタン距離をヒューリスティックとしてスタートと目標セル間の最短経路を計算する。
// また、グリッド座標とワールド座標間の変換や、直線の見通しが利く場合に不要な中間
// ウェイポイントを削除してパスを平滑化するヘルパー関数も提供する。
class Pathfinder
{
public:

    static std::vector<GridCoord> FindPath(const MakeMap* map,
        const GridCoord& start,
        const GridCoord& goal);

    static Vector3 GridToWorld(const MakeMap* map,
        const GridCoord& coord);

    static GridCoord WorldToGrid(const MakeMap* map,const Vector3& pos);

    static std::vector<Vector3> SmoothPath(
        const std::vector<Vector3>& path,
        const std::function<bool(const Vector3&, const Vector3&)>& isWalkable);

private:
    // A*ヒューリスティック用の内部ヘルパー（マンハッタン距離）
    static float Heuristic(const GridCoord& a, const GridCoord& b)
    {
        return static_cast<float>(std::abs(a.x - b.x) + std::abs(a.y - b.y));
    }
};