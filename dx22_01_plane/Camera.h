#pragma once
#include	<SimpleMath.h>
#include<vector>
#include"Collider.h"
#include"Terrain.h"
//-----------------------------------------------------------------------------
//Cameraクラス
//-----------------------------------------------------------------------------
class Camera {
protected:
	DirectX::SimpleMath::Vector3	m_Position = DirectX::SimpleMath::Vector3(0.0f, 50.0f, 0.0f);
	DirectX::SimpleMath::Vector3	m_Rotation = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f);
	DirectX::SimpleMath::Vector3	m_Scale = DirectX::SimpleMath::Vector3(1.0f, 1.0f, 1.0f);
	DirectX::SimpleMath::Matrix		m_ViewMatrix{};
	DirectX::SimpleMath::Matrix m_ProjectionMatrix;
	DirectX::SimpleMath::Vector3	m_Target{};//カメラの注視点

	// 一人称視点用の回転角度
	float m_Yaw = 0.0f;          // 水平回転角度（ヨー）
	float m_Pitch = 0.0f;        // 垂直回転角度（ピッチ）
	float m_MouseSensitivity = 0.005f;  // マウス感度
	bool m_MouseCaptured = true; // マウスキャプチャ状態
	bool m_EscKeyPressed = false; // ESCキーが押されたかのフラグ

	DirectX::SimpleMath::Vector3 m_Velocity{};//移動速度
	Collider m_Collider;//当たり判定

private:

	HCURSOR m_DefaultCursor = nullptr;

public:
	void Init();
	void Update();
	void Draw();
	void Uninit();
	void SetCamera(int mode);//カメラの設定
	void SetPosition(const DirectX::SimpleMath::Vector3& pos) { m_Position = pos; }

	DirectX::SimpleMath::Vector3 GetPosition() const { return m_Position; }
	DirectX::SimpleMath::Matrix GetViewMatrix()const { return m_ViewMatrix; }
	DirectX::SimpleMath::Matrix GetProjectionMatrix()const { return m_ProjectionMatrix; }

	// マウス感度を設定するメソッド
	void SetMouseSensitivity(float sensitivity) { m_MouseSensitivity = sensitivity; }
	float GetMouseSensitivity() const { return m_MouseSensitivity; }

	// マウスキャプチャ状態を取得
	bool IsMouseCaptured() const { return m_MouseCaptured; }
};