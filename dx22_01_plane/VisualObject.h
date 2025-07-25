#pragma once
#include"Object.h"	

class VisualObject :public Object
{
protected:

	// 描画情報
	IndexBuffer m_IndexBuffer; // インデックスバッファ
	VertexBuffer<VERTEX_3D> m_VertexBuffer; // 頂点バッファ

	// 描画情報
	Texture m_Texture; // テクスチャ
	std::unique_ptr<Material> m_Materiale; //マテリアル

	// UV座標の情報
	float m_NumU = 1;
	float m_NumV = 1;
	float m_SplitX = 1;
	float m_SplitY = 1;

public:
	VisualObject(
		const DirectX::SimpleMath::Vector3& pos = { 0.0f, 0.0f, 0.0f },
		const DirectX::SimpleMath::Vector3& size = { 1.0f, 1.0f, 1.0f });

	 ~VisualObject();

	void Init() override;
	void Update() override;
	void Draw() override;
	void Uninit() override;

	bool Is3D() const override { return false; } // 2D描画

	// テクスチャを指定
	void SetTexture(const char* imgname);

	// 位置を指定
	void SetPosition(const float& x, const float& y, const float& z);
	void SetPosition(const DirectX::SimpleMath::Vector3& pos);

	// 角度を指定
	void SetRotation(const float& x, const float& y, const float& z);
	void SetRotation(const DirectX::SimpleMath::Vector3& rot);

	// 大きさを指定
	void SetScale(const float& x, const float& y, const float& z);
	void SetScale(const DirectX::SimpleMath::Vector3& scl);

	// UV座標を指定
	void SetUV(const float& nu, const float& nv, const float& sx, const float& sy);
};

