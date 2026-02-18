#include "pch.h"
#include "ColliderObject.h"
#include"Game.h"

void ColliderObject::Draw()
{

	Object::Draw();// 通常の描画

#ifdef _DEBUG

	//collider.DrawDebugCollider(Game::GetInstance().GetMainCamera()); // デバッグ描画

#endif // _DEBUG

}


void ColliderObject::SetPosition(const DirectX::SimpleMath::Vector3& pos)
{
	Object::SetPosition(pos);
	collider.center = pos; // コライダーの中心位置を更新
}