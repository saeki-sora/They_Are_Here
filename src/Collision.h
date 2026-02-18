#pragma once

namespace Collision
{
	// ライン（無限の長さの線）
	struct Line {
		DirectX::SimpleMath::Vector3 point; // 通過地点
		DirectX::SimpleMath::Vector3 vec; // 線の方向ベクトル
	};

	// プレーン(無限の広さの平面)
	struct Plane {
		DirectX::SimpleMath::Vector3 point; // 平面上の1点
		DirectX::SimpleMath::Vector3 normal; // 平面の法線ベクトル
		//float d;         // 平面の方程式: ax + by + cz + d = 0 の d
	};

	// セグメント（有限の長さの線分）
	struct Segment {
		DirectX::SimpleMath::Vector3 start; // 始点
		DirectX::SimpleMath::Vector3 end; // 終点
	};

	// 三角形ポリゴン（有限の広さの平面）
	struct Polygon {
		const DirectX::SimpleMath::Vector3 p0; //頂点0
		const DirectX::SimpleMath::Vector3 p1; //頂点1
		const DirectX::SimpleMath::Vector3 p2; //頂点2
	};

	// 球体
	struct Sphere {
		DirectX::SimpleMath::Vector3 center; // 中心
		float radius; // 半径
	};

	// 円柱
	struct Cylinder {
		DirectX::SimpleMath::Vector3 top; // 上
		DirectX::SimpleMath::Vector3 bottom; // 底
		float radius; // 半径
	};

	// カプセル
	struct Capsule {
		DirectX::SimpleMath::Vector3 top; // 上の中心 
		DirectX::SimpleMath::Vector3 bottom; // 底の中心
		float radius; // 半径
	};

	// BOX定義
	struct AABB {
		DirectX::SimpleMath::Vector3 min;
		DirectX::SimpleMath::Vector3 max;
	};

	//当たり判定
	bool CheckHit(const Line& line, const Plane& plane); //線(無限の長さ)と平面(無限の大きさ)
	bool CheckHit(const Segment& segment, const Plane& plane); //線分と平面(無限の大きさ)
	bool CheckHit(const Line& line, const Polygon& polygon); //線(無限の長さ)とポリゴン
	bool CheckHit(const Line& line, const Polygon& polygon, DirectX::SimpleMath::Vector3& contact); //同上
	bool CheckHit(const Segment& segment, const Polygon& polygon); //線分とポリゴン
	bool CheckHit(const Segment& segment, const Polygon& polygon, DirectX::SimpleMath::Vector3& contact); //同上
	bool CheckHit(const Sphere& sphere, const Plane& plane); //球体と平面(無限の大きさ)
	bool CheckHit(const Sphere& sphere, const Polygon& polygon); //球体とポリゴン
	bool CheckHit(const Sphere& sphere, const Polygon& polygon, DirectX::SimpleMath::Vector3& contact); //同上
	bool CheckHit(const Cylinder& cylinder, const Plane& plane); //円柱と平面
	bool CheckHit(const Cylinder& cylinder, const Polygon& polygon); //円柱とポリゴン
	bool CheckHit(const Cylinder& cylinder, const Polygon& polygon, DirectX::SimpleMath::Vector3& contact); //同上
	bool CheckHit(const Capsule& capsule, const Plane& plane); //カプセルと平面(無限の大きさ)
	bool CheckHit(const Capsule& capsule, const Polygon& polygon); //カプセルとポリゴン
	bool CheckHit(const Capsule& capsule, const Polygon& polygon, DirectX::SimpleMath::Vector3& contact); //同上
	bool CheckHit(Sphere sphere1, Sphere sphere2); //球体と球体
	bool CheckHit(Sphere sphere1, Sphere sphere2, DirectX::SimpleMath::Vector3& contact); //同上
	bool CheckHit(AABB p1, AABB p2); // AABBとAABB


	//内積・外積
	float Dot(const DirectX::SimpleMath::Vector3& v1, const DirectX::SimpleMath::Vector3& v2);
	DirectX::SimpleMath::Vector3 Cross(const DirectX::SimpleMath::Vector3& v1, const DirectX::SimpleMath::Vector3& v2);


	DirectX::SimpleMath::Vector3 ClosestPointOnSegment(const DirectX::SimpleMath::Vector3& point, const Segment& segment);
	float DistanceSquaredPointToSegment(const DirectX::SimpleMath::Vector3& point, const Segment& segment);
	float DistanceSquaredPointToSegment(const DirectX::SimpleMath::Vector3& point, const Segment& segment, DirectX::SimpleMath::Vector3& contact);
	float DistancePointToSegment(const DirectX::SimpleMath::Vector3& point, const Segment& segment);
	float DistancePointToSegment(const DirectX::SimpleMath::Vector3& point, const Segment& segment, DirectX::SimpleMath::Vector3& contact);
	float DistancePointToPlane(const DirectX::SimpleMath::Vector3& point, const Plane& plane);
	DirectX::SimpleMath::Vector3 ProjectPointToPlane(const DirectX::SimpleMath::Vector3& point, const Plane& plane);
	bool PointInTriangle(const DirectX::SimpleMath::Vector3& point, const Polygon& polygon);
	DirectX::SimpleMath::Vector3 ClosestPointOnTriangle(const DirectX::SimpleMath::Vector3& point, const Polygon& polygon);
	DirectX::SimpleMath::Vector3 GetNormal(const Polygon& polygon);


	DirectX::SimpleMath::Vector3 moveSphere(const Segment& capsule, const float& radius, const Polygon& polygon, const DirectX::SimpleMath::Vector3& contact, float& distance);
	DirectX::SimpleMath::Vector3 moveSphere(const Sphere& sphere, const Polygon& polygon, const DirectX::SimpleMath::Vector3& contact);

	// make AABB
	AABB SetAABB(DirectX::SimpleMath::Vector3 centerposition, float width, int height, int depth);

/*
	// 線分に平行な与えられた幅の４角形の座標を求める
	void CalcQuadOrientedLine(
		DirectX::SimpleMath::Vector3 startpos,
		DirectX::SimpleMath::Vector3 endpos,
		DirectX::SimpleMath::Vector3* v,
		float width);


	// 幅をもった線分（四角形）と球の当たり判定を行う
	bool CollisionSphereOrientedQuad(
		DirectX::SimpleMath::Vector3 StartPos,						// 開始位置
		DirectX::SimpleMath::Vector3 EndPos,							// 終了位置
		float width,							// 幅
		float radius,							// 半径
		DirectX::SimpleMath::Vector3 CirclePos);						// 中心座標
	*/

}