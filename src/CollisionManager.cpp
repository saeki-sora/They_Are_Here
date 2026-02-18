#include "pch.h"
#include "CollisionManager.h"

bool CollisionManager::CheckCollisionWithAny(
	const std::weak_ptr<ColliderObject>& target,
	const std::vector<std::shared_ptr<ColliderObject>>& objects)
{
	if (auto targetPtr = target.lock())
	{
		for (const auto& obj : objects)
		{
			if (obj && CheckPair(target, obj))
			{
				return true;
			}
		}
	}
	return false;
}

ColliderObject* CollisionManager::GetCollidedObject(
	const std::weak_ptr<ColliderObject>& target,
	const std::vector<std::shared_ptr<ColliderObject>>& objects)
{
	if (auto targetPtr = target.lock())
	{
		for (const auto& obj : objects)
		{
			if (obj && CheckPair(target, obj))
			{
				return obj.get();
			}
		}
	}
	return nullptr;
}

bool CollisionManager::CheckPair(const std::weak_ptr<ColliderObject>& a, const std::weak_ptr<ColliderObject>& b)
{
	if (auto aPtr = a.lock())
	{
		if (auto bPtr = b.lock())
		{
			return aPtr->GetCollider().CheckCollision(bPtr->GetCollider());
		}
	}
	return false;
}