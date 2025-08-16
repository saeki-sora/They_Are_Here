#include "Renderer.h"
#include "Camera.h"
#include "Application.h"
#include"input.h"
#include"Game.h"
#include "Player.h"

using namespace DirectX::SimpleMath;

//=======================================
//初期化処理
//=======================================
void Camera::Init()
{
	m_Position = Vector3(0.0f, 40.0f, -50.0f);
	m_Target = Vector3(0.0f, 0.0f, 0.0f);
	m_Yaw = 0.0f;        // 水平回転角度（ヨー）
	m_Pitch = 0.0f;      // 垂直回転角度（ピッチ）
	m_MouseSensitivity = 0.005f;  // マウス感度
	m_MouseCaptured = true;       // 初期状態はマウスキャプチャON
	m_EscKeyPressed = false;      // ESCキーフラグ初期化
	m_CameraOffset = Vector3(0.0f, 10.0f, 0.0f); // カメラのオフセット（プレイヤーからの相対位置）


	// デフォルトカーソルを保存
	m_DefaultCursor = LoadCursor(NULL, IDC_ARROW);

	// マウスカーソルを非表示にする
	while (ShowCursor(FALSE) >= 0);

}

//=======================================
//更新処理
//=======================================
void Camera::Update()
{

	// ESCキーでマウスキャプチャの切り替えと終了確認
	bool escKeyCurrentlyPressed = Input::GetKeyPress(VK_ESCAPE);
	if (escKeyCurrentlyPressed && !m_EscKeyPressed)
	{
		// マウスキャプチャがONの場合、カーソルを表示して終了確認ダイアログを表示
		if (m_MouseCaptured)
		{
			// マウスキャプチャOFF：カーソル表示
			m_MouseCaptured = false;
			while (ShowCursor(TRUE) < 0);   // カーソルを表示する
			SetCursor(m_DefaultCursor);
		}

		// 終了確認ダイアログを表示（FindWindowでウィンドウハンドルを取得）
		HWND hWnd = FindWindow(TEXT("2024 framework ひな型"), TEXT("2024 framework ひな型(フィールド描画)"));
		if (hWnd != nullptr)
		{
			PostMessage(hWnd, WM_CLOSE, 0, 0);
		}
	}
	m_EscKeyPressed = escKeyCurrentlyPressed;

	// マウスキャプチャがOFFの時、左クリックでマウスキャプチャを再開
	if (!m_MouseCaptured && Input::GetKeyPress(VK_LBUTTON))
	{
		m_MouseCaptured = true;
		while (ShowCursor(FALSE) >= 0); // カーソルを完全に非表示にする
		SetCursor(NULL);
	}

	// プレイヤーオブジェクトを取得して状態を同期
	if (auto player = Game::GetInstance().FindObject<Player>().lock())
	{
		// プレイヤーのマウスキャプチャ状態をカメラと同期
		player->SetMouseCaptured(m_MouseCaptured);

		// カメラの位置を設定（プレイヤーの目の高さ）
		Vector3 eyeOffset = Vector3(0.0f, 8.0f, 0.0f);
		m_Position = player->GetPosition() + eyeOffset;

		// プレイヤーのYaw（水平回転）とPitch（垂直視点）を取得
		float playerYaw = player->GetYawRotation();
		float playerPitch = player->GetPitch();

		// カメラの前方向ベクトルを計算（ヨーとピッチの両方を使用）
		Vector3 forward;
		forward.x = sin(playerYaw) * cos(playerPitch);
		forward.y = sin(playerPitch); 
		forward.z = cos(playerYaw) * cos(playerPitch);
		forward.Normalize();

		// カメラのターゲットを設定
		float lookDistance = 100.0f;
		m_Target = m_Position + (forward * lookDistance);
	}

}

//=======================================
//描画処理
//=======================================
void Camera::Draw()
{

}

//=======================================
//終了処理
//=======================================
void Camera::Uninit() {}

//=======================================
//カメラの設定
//=======================================
void Camera::SetCamera(int mode)
{
	//3D
	if (mode == 0)
	{
		// ビュー変換後列作成
		Vector3 up = Vector3(0.0f, 1.0f, 0.0f);
		m_ViewMatrix = DirectX::XMMatrixLookAtLH(m_Position, m_Target, up); // 左手系にした　20230511 by suzuki.tomoki
		// DIRECTXTKのメソッドは右手系　20230511 by suzuki.tomoki
		// 右手系にすると３角形頂点が反時計回りになるので描画されなくなるので注意
		// このコードは確認テストのために残す
		// m_ViewMatrix = m_ViewMatrix.CreateLookAt(m_Position, m_Target, up);		

		Renderer::SetViewMatrix(&m_ViewMatrix);

		//プロジェクション行列の生成
		constexpr float fieldOfView = DirectX::XMConvertToRadians(90.0f);    // 視野角

		float aspectRatio = static_cast<float>(Application::GetWidth()) / static_cast<float>(Application::GetHeight());	// アスペクト比
		float nearPlane = 0.1f;       // ニアクリップ
		float farPlane = 1000.0f;      // ファークリップ

		//プロジェクション行列の生成
		m_ProjectionMatrix = DirectX::XMMatrixPerspectiveFovLH(fieldOfView, aspectRatio, nearPlane, farPlane);	// 左手系にした　20230511 by suzuki.tomoki
		// DIRECTXTKのメソッドは右手系　20230511 by suzuki.tomoki
		// 右手系にすると３角形頂点が反時計回りになるので描画されなくなるので注意
		// このコードは確認テストのために残す
		// projectionMatrix = DirectX::SimpleMath::Matrix::CreatePerspectiveFieldOfView(fieldOfView, aspectRatio, nearPlane, farPlane);

		Renderer::SetProjectionMatrix(&m_ProjectionMatrix);

	}
	else if (mode == 1)//2D
	{
		Vector3 pos = { 0.0f,0.0f,-10.0f };
		Vector3 tgt = { 0.0f,0.0f,1.0f };
		Vector3 up = Vector3(0.0f, 1.0f, 0.0f);
		m_ViewMatrix = DirectX::XMMatrixLookAtLH(pos, tgt, up); // 左手系にした　20230511 by suzuki.tomoki
		Renderer::SetViewMatrix(&m_ViewMatrix);

		//プロジェクション行列の生成
		float nearPlane = 1.0f;       // ニアクリップ
		float farPlane = 1000.0f;      // ファークリップ
		Matrix projectionMatrix = DirectX::XMMatrixOrthographicLH(static_cast<float>(Application::GetWidth()),
			static_cast<float>(Application::GetHeight()), nearPlane, farPlane);	// 左手系にした　20230511 by suzuki.tomoki

		projectionMatrix = DirectX::XMMatrixTranspose(projectionMatrix);
		Renderer::SetProjectionMatrix(&projectionMatrix);
	}
}