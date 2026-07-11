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

	std::vector<VERTEX_3D> m_BaseVertices; // SetAlphaでの頂点色書き換え用に保持する基本頂点データ
	float m_Alpha = 1.0f; // 不透明度（フェード演出用）

public:

	VisualObject(
		const DirectX::SimpleMath::Vector3& pos = { 0.0f, 0.0f, 0.0f },
		const DirectX::SimpleMath::Vector3& size = { 1.0f, 1.0f, 1.0f });

	 ~VisualObject();

	void Init() override;
	void Update(float deltaTime) override;
	void Draw() override;
	void Uninit() override;

	bool Is3D() const override { return false; } // 2D描画

	// テクスチャを指定
	void SetTexture(const char* imgname);

	// シェーダーを指定
	void SetShader(const char* vsPath, const char* psPath);

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

	// 不透明度を指定（0.0〜1.0、フェードイン・フェードアウト演出用）
	void SetAlpha(float alpha);
};

