#pragma once
#include <SimpleMath.h>
#include "Collision.h"

// 当たり判定用の直方体（AABB）を扱うクラス
class Collider {
public:

	DirectX::SimpleMath::Vector3 position;// 直方体の中心位置

	DirectX::SimpleMath::Vector3 size;// 直方体の半分のサイズ

	// 現在の位置とサイズを元にAABBを生成
	Collision::AABB GetAABB() const;
};
