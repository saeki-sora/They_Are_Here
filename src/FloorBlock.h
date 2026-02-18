#pragma once
#include "Object.h"
#include "MeshRenderer.h"
#include "Texture.h"
#include "Material.h"
#include"VisualObject.h"

class FloorBlock :public VisualObject
{
public:

	FloorBlock(
		const DirectX::SimpleMath::Vector3& pos = { 0, 50, 0 },
		const DirectX::SimpleMath::Vector3& size = { 10, 10, 10 });//コンストラクタ

	~FloorBlock(); // デストラクタ

	void Init()override;
	void Update(float deltaTime)override;
	void Draw()override;
	void Uninit()override;

	bool Is3D() const override { return true; }

};

