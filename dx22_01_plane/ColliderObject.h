#pragma once
#include"Object.h"
#include <DirectXColors.h>
#include <PrimitiveBatch.h>
#include <VertexTypes.h>
#include <CommonStates.h>
#include <Effects.h>

class ColliderObject : public Object
{
	// ColliderObjectクラスはObjectクラスを継承し、衝突判定用のコライダーを持つオブジェクトを表す
protected:

	SimpleBoxCollider collider;

public:

	//コンストラクタ
	ColliderObject(
		const DirectX::SimpleMath::Vector3& pos,
		const DirectX::SimpleMath::Vector3& size) :
		Object(pos, size),
		collider(pos, size) {}

	// 省略版コンストラクタ
	ColliderObject(Camera* cam)
		: ColliderObject({ 0,0,0 }, { 10,1,10 }) {}

	virtual ~ColliderObject() {}

	bool Is3D() const override { return true; } // 明示的に3D

	const SimpleBoxCollider& GetCollider() const { return collider; }

	void SetPosition(const DirectX::SimpleMath::Vector3& pos);

	void SetScale(const DirectX::SimpleMath::Vector3& scale) override
	{
		Object::SetScale(scale);
		collider.size = scale;
	}

	void Draw() override;
};

