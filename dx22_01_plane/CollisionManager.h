#pragma once
#include "ColliderObject.h"
#include <vector>
#include <memory>

class CollisionManager
{
public:

    //•¡”‚Æ‚Ì“–‚½‚è”»’è(‚Ç‚ê‚©‚P‚Â‚Å‚à“–‚½‚Á‚½‚çtrue)
    static bool CheckCollisionWithAny(
        const std::weak_ptr<ColliderObject>& target,
        const std::vector<std::shared_ptr<ColliderObject>>& objects);

    //‰½‚Æ“–‚½‚Á‚½‚©‚ğ•Ô‚·
    static ColliderObject* GetCollidedObject(
        const std::weak_ptr<ColliderObject>& target,
        const std::vector<std::shared_ptr<ColliderObject>>& objects);

    // ˆê‘Îˆê‚Ì“–‚½‚è”»’è
    static bool CheckPair(const std::weak_ptr<ColliderObject>& a, const std::weak_ptr<ColliderObject>& b);
};
