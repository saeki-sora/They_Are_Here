#pragma once
#include "ColliderObject.h"


class CollisionManager
{
public:

    //複数との当たり判定(どれか１つでも当たったらtrue)
    static bool CheckCollisionWithAny(
        const std::weak_ptr<ColliderObject>& target,
        const std::vector<std::shared_ptr<ColliderObject>>& objects);

    //何と当たったかを返す
    static ColliderObject* GetCollidedObject(
        const std::weak_ptr<ColliderObject>& target,
        const std::vector<std::shared_ptr<ColliderObject>>& objects);

    // 一対一の当たり判定
    static bool CheckPair(const std::weak_ptr<ColliderObject>& a, const std::weak_ptr<ColliderObject>& b);
};
