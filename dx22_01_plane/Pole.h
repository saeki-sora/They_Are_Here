#pragma once
#include "Object.h"
#include "MeshRenderer.h"
#include "Texture.h"
#include "Material.h"
#include"ColliderObject.h"

class Pole :public ColliderObject
{
private:

	// 描画の為の情報（メッシュに関わる情報）
	IndexBuffer	 m_IndexBuffer; // インデックスバッファ
	VertexBuffer<VERTEX_3D>	m_VertexBuffer; // 頂点バッファ

	Texture m_Texture;//テクスチャ
	std::unique_ptr<Material> m_Material; //マテリアル

	bool IsGoal = false; //ゴールフラグ

public:

	Pole(
		const DirectX::SimpleMath::Vector3& pos = { 0, 50, 0 },
		const DirectX::SimpleMath::Vector3& size = { 10, 10, 10 });//コンストラクタ

	~Pole(); // デストラクタ

	void Init()override;
	void Update()override;
	void Draw()override;
	void Uninit()override;

	bool GetGoalFlag() const { return IsGoal; }// ゴールフラグの取得
	void SetPosition(const DirectX::SimpleMath::Vector3& pos) { m_Position = pos; }

};

