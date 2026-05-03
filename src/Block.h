#pragma once
#include "Object.h"
#include "MeshRenderer.h"
#include "Texture.h"
#include "Material.h"
#include"ColliderObject.h"
#include "MakeMap.h"

class Block :public ColliderObject
{
private:

	// 自身が MakeMap 上のどのセルにいるか
	int m_gridX = -1;
	int m_gridY = -1;

	BlockType m_type; // 自分のタイプを保存する変数

	// 全インスタンスで共有する GPU バッファ（FBX は1回だけロード）
	static MeshRenderer                           s_SharedRenderer;
	static bool                                   s_MeshReady;
	static DirectX::SimpleMath::Vector3           s_ColliderExtents;
	static std::vector<SUBSET>                    s_SharedSubsets;
	static std::vector<MATERIAL>                  s_SharedMaterials;
	static std::vector<std::shared_ptr<Texture>>  s_NormalTextures;

public:

	Block(
		const DirectX::SimpleMath::Vector3& pos = { 0, 50, 0 },
		const DirectX::SimpleMath::Vector3& size = { 10, 10, 10 },
		BlockType type = BlockType::Normal);//コンストラクタ

	~Block(); // デストラクタ

	void Init()override;
	void Update(float deltaTime)override;
	void Draw()override;
	void DrawShadow()override;
	void Uninit()override;

	// グリッド座標を設定する
	void SetGridCoords(int x, int y) { m_gridX = x; m_gridY = y; }
	// グリッド座標を取得する
	int GetGridX() const { return m_gridX; }
	int GetGridY() const { return m_gridY; }
};

