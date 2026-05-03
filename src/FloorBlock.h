#pragma once
#include "Object.h"
#include "MeshRenderer.h"
#include "Texture.h"
#include "Material.h"
#include"VisualObject.h"

class FloorBlock :public VisualObject
{
private:
	// 全インスタンスで共有する GPU バッファ（FBX は1回だけロード）
	static MeshRenderer                           s_SharedRenderer;
	static bool                                   s_MeshReady;
	static std::vector<SUBSET>                    s_SharedSubsets;
	static std::vector<MATERIAL>                  s_SharedMaterials;
	static std::vector<std::shared_ptr<Texture>>  s_SharedTextures;

public:

	FloorBlock(
		const DirectX::SimpleMath::Vector3& pos = { 0, 50, 0 },
		const DirectX::SimpleMath::Vector3& size = { 10, 10, 10 });//コンストラクタ

	~FloorBlock(); // デストラクタ

	void Init()override;
	void Update(float deltaTime)override;
	void Draw()override;
	void DrawShadow()override;
	void Uninit()override;

	bool Is3D() const override { return true; }

};

