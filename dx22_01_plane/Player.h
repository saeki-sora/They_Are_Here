#pragma once
#include "Object.h"
#include "MeshRenderer.h"
#include "Texture.h"
#include "Material.h"
#include"ColliderObject.h"

#include <SimpleMath.h>

using Vector3 = DirectX::SimpleMath::Vector3;

class Player :public ColliderObject
{
private:

	// 描画の為の情報（メッシュに関わる情報）
	IndexBuffer	 m_IndexBuffer; // インデックスバッファ
	VertexBuffer<VERTEX_3D>	m_VertexBuffer; // 頂点バッファ
	Texture m_Texture;//テクスチャ
	std::unique_ptr<Material> m_Material; //マテリアル

	Vector3 m_Forward; // 前方ベクトル

	float m_MoveSpeed; // 移動速度
	float m_MouseSensitivity;  // マウス感度
	bool m_MouseCaptured = true; // マウスキャプチャ状態

	bool IsGoal = false; //ゴールフラグ

public:

	Player(
		const DirectX::SimpleMath::Vector3& pos = { 0, 50, 0 },
		const DirectX::SimpleMath::Vector3& size = { 10, 10, 10 });//コンストラクタ

	~Player(); // デストラクタ

	void Init()override;
	void Update()override;
	void Draw()override;
	void Uninit()override;

	bool GetGoalFlag() const { return IsGoal; }// ゴールフラグの取得

	Vector3 GetPosition() const override { return m_Position; } // ポジションの取得
	void SetPosition(const DirectX::SimpleMath::Vector3& pos) { m_Position = pos; }

	// マウス感度を設定するメソッド
	void SetMouseSensitivity(float sensitivity) { m_MouseSensitivity = sensitivity; }
	float GetMouseSensitivity() const { return m_MouseSensitivity; }

	Vector3 GetForward() const { return m_Forward; } // 前方ベクトルの取得

	void SetMouseCaptured(bool captured) { m_MouseCaptured = captured; }
	bool IsMouseCaptured() const { return m_MouseCaptured; }

};

