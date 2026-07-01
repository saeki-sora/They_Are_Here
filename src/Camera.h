#pragma once

using namespace DirectX::SimpleMath;
//-----------------------------------------------------------------------------
//Cameraクラス
//-----------------------------------------------------------------------------
class Camera 
{
public:

	enum class Mode
	{
		Stop,           //動かせない
		Free,			//自由移動（デバッグ用）
		FollowPlayer	// プレイヤーを追従（ゲーム本編）
	};

protected:

	DirectX::SimpleMath::Vector3	m_Position = DirectX::SimpleMath::Vector3(0.0f, 50.0f, 0.0f);
	DirectX::SimpleMath::Vector3	m_Rotation = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f);
	DirectX::SimpleMath::Vector3	m_Scale = DirectX::SimpleMath::Vector3(1.0f, 1.0f, 1.0f);
	DirectX::SimpleMath::Matrix		m_ViewMatrix{};
	DirectX::SimpleMath::Matrix m_ProjectionMatrix;
	DirectX::SimpleMath::Vector3	m_Target{};//カメラの注視点

	float m_Yaw = 0.0f;         // 水平回転角度（ヨー）
	float m_Pitch = 0.0f;         // 垂直回転角度（ピッチ）
	float m_MouseSensitivity = 0.005f;     // マウス感度
	bool  m_MouseCaptured = true;         // マウスキャプチャ状態
	bool  m_ClickToRecapture = true;      // 左クリックでキャプチャを復帰させるか（ポーズ中はfalse）
	Vector3 m_CameraOffset; // カメラのオフセット

	float m_WalkTimer = 0.0f;         // 歩行揺れ計算用タイマー
	float m_BobbingSpeed = 14.0f;     // 揺れの速さ
	float m_BobbingAmount = 3.5f;     // 揺れの大きさ（高さ）

	// カメラ補間移動用
	bool   m_IsInterpolating = false;   // 補間中フラグ
	Vector3 m_InterpolateStartPos;      // 補間開始位置
	Vector3 m_InterpolateEndPos;        // 補間終了位置
	Vector3 m_InterpolateStartTarget;   // 補間開始ターゲット（注視点）
	Vector3 m_InterpolateEndTarget;     // 補間終了ターゲット（注視点）
	float   m_InterpolateTimer = 0.0f;  // 補間経過時間（秒）
	float   m_InterpolateDuration = 0.0f; // 補間総時間（秒）

	Mode m_CurrentMode = Mode::FollowPlayer;//デフォルトはプレイヤー追従モード

	DirectX::BoundingFrustum m_Frustum;//カリング用フラスタム

	float m_FreeMoveSpeed = 20.0f; // Freeモード時の移動速度

private:

	HCURSOR m_DefaultCursor = nullptr;
	bool m_JustCaptured = false;// マウスキャプチャ直後フラグ

public:

	void Init();
	void Update(float deltaTime);
	void Draw();
	void Uninit();

	void SetCamera(int mode);//カメラの設定
	void SetPosition(const DirectX::SimpleMath::Vector3& pos) { m_Position = pos; }
	void SetTarget(const DirectX::SimpleMath::Vector3& target) { m_Target = target; }
	void SetLookAt(const Vector3& pos, const Vector3& target);
	void SetMouseCaptured(bool captured) { m_MouseCaptured = captured; }
	void SetClickToRecapture(bool allow) { m_ClickToRecapture = allow; } // ポーズ中はfalseにしてクリックでの誤キャプチャを防ぐ
	void RecaptureMouseImmediate(); // ポーズ解除時に即マウス再キャプチャ
	void ReleaseMouseImmediate();   // ポーズ開始時にマウスを解放

	DirectX::SimpleMath::Vector3 GetPosition() const { return m_Position; }
	DirectX::SimpleMath::Matrix GetViewMatrix()const { return m_ViewMatrix; }
	DirectX::SimpleMath::Matrix GetProjectionMatrix()const { return m_ProjectionMatrix; }
	DirectX::SimpleMath::Vector3 GetTarget() const { return m_Target; }
	POINT GetWindowCenterScreen() const;
	const DirectX::BoundingFrustum & GetFrustum() const { return m_Frustum; }//視錐台を取得するゲッター
	
	//カメラの感度を設定するメソッド
	void SetMouseSensitivity(float sensitivity) { m_MouseSensitivity = sensitivity; }
	float GetMouseSensitivity() const { return m_MouseSensitivity; }

	// マウスキャプチャ状態を取得
	bool IsMouseCaptured() const { return m_MouseCaptured; }

	void SetCameraOffset(const Vector3& offset) { m_CameraOffset = offset; }

	// 補間移動中かどうか
	bool IsInterpolating() const { return m_IsInterpolating; }

	void CenterCursorToWindow();// ウィンドウ中央にカーソルを移動

	void SetMode(Mode mode) { m_CurrentMode = mode; }
	Mode GetMode() const { return m_CurrentMode; }
};