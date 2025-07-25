#include "Collider.h"

Collision::AABB Collider::GetAABB()const {

	return {
		position - size, // min座標（左下奥）
		position + size  // max座標（右上手前）
	};
}