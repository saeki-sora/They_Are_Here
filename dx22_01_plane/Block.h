#pragma once
#include "Object.h"
#include "MeshRenderer.h"
#include "Texture.h"
#include "Material.h"
#include"ColliderObject.h"

class Block :public ColliderObject
{
private:

	// 描画の為の情報（メッシュに関わる情報）
	IndexBuffer	 m_IndexBuffer; // インデックスバッファ
	VertexBuffer<VERTEX_3D>	m_VertexBuffer; // 頂点バッファ

	Texture m_Texture;//テクスチャ
	std::unique_ptr<Material> m_Material; //マテリアル
	std::vector<VERTEX_3D> m_Vertices;// 頂点情報

public:

	Block(
		const DirectX::SimpleMath::Vector3& pos = { 0, 50, 0 },
		const DirectX::SimpleMath::Vector3& size = { 10, 10, 10 });//コンストラクタ

	~Block(); // デストラクタ

	void Init()override;
	void Update()override;
	void Draw()override;
	void Uninit()override;

	std::vector<VERTEX_3D> GetVertices() { return m_Vertices; }
};

