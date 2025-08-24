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

	float m_Pitch; // 垂直回転角度（ピッチ）
	Vector3 m_Forward; // 前方ベクトル

	float m_MoveSpeed; // 移動速度
	float m_MouseSensitivity;  // マウス感度
	bool m_MouseCaptured = true; // マウスキャプチャ状態

	bool IsGoal = false; //ゴールフラグ

public:

	Player(
		const DirectX::SimpleMath::Vector3& pos,
		const DirectX::SimpleMath::Vector3& size);//コンストラクタ

	~Player(); // デストラクタ

	void Init()override;
	void Update()override;
	void Draw()override;
	void Uninit()override;

	bool GetGoalFlag() const { return IsGoal; }// ゴールフラグの取得

	bool CheckCollisionWithBlocks(const Vector3& newPosition);// ブロックとの衝突判定

	Vector3 GetPosition() const override { return m_Position; } // ポジションの取得
	void SetPosition(const DirectX::SimpleMath::Vector3& pos) { m_Position = pos; }

	Vector3 GetRotation() const { return m_Rotation; }// 回転の取得
	void SetRotation(const Vector3& rotation) { m_Rotation = rotation; }

	float GetYawRotation() const { return m_Rotation.y; } // 水平回転角度（ヨー）の取得
	void SetYawRotation(float yaw) { m_Rotation.y = yaw; }

	float GetPitch() const { return m_Pitch; }// 垂直回転角度（ピッチ）の取得
	void SetPitch(float pitch) { m_Pitch = pitch; }

	// マウス感度を設定するメソッド
	void SetMouseSensitivity(float sensitivity) { m_MouseSensitivity = sensitivity; }
	float GetMouseSensitivity() const { return m_MouseSensitivity; }

	Vector3 GetForward() const { return m_Forward; } // 前方ベクトルの取得

	void SetMouseCaptured(bool captured) { m_MouseCaptured = captured; }
	bool IsMouseCaptured() const { return m_MouseCaptured; }

};

