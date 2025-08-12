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

	//// マウスキャプチャ状態の時のみマウス操作を処理
	//if (m_MouseCaptured)
	//{
	//	// マウスの移動量を取得
	//	int mouseX, mouseY;
	//	Input::GetMouseMove(&mouseX, &mouseY);

	//	// マウスの移動量からカメラの回転を更新
	//	m_Yaw += mouseX * m_MouseSensitivity;    // 水平回転
	//	m_Pitch -= mouseY * m_MouseSensitivity;  // 垂直回転（Y軸は反転）

	//	// ピッチ角度を制限（真上・真下を向きすぎないよう制限）
	//	const float maxPitch = DirectX::XM_PI / 2.0f - 0.1f; // 約89度
	//	if (m_Pitch > maxPitch) m_Pitch = maxPitch;
	//	if (m_Pitch < -maxPitch) m_Pitch = -maxPitch;
	//}

	//// カメラの前方向ベクトルを計算（球面座標系から直交座標系へ変換）
	//Vector3 forward;
	//forward.x = cos(m_Pitch) * sin(m_Yaw);
	//forward.y = sin(m_Pitch);
	//forward.z = cos(m_Pitch) * cos(m_Yaw);
	//forward.Normalize();

	//// 右方向ベクトルを計算
	//Vector3 right = Vector3(cos(m_Yaw), 0.0f, -sin(m_Yaw));
	//right.Normalize();

	//// WASDでカメラの移動
	//Vector3 moveVector = Vector3(0.0f, 0.0f, 0.0f);
	//const float moveSpeed = 0.5f;

	//if (Input::GetKeyPress('W'))
	//{
	//	moveVector += forward * moveSpeed;
	//}
	//if (Input::GetKeyPress('S'))
	//{
	//	moveVector -= forward * moveSpeed;
	//}
	//if (Input::GetKeyPress('A'))
	//{
	//	moveVector -= right * moveSpeed;
	//}
	//if (Input::GetKeyPress('D'))
	//{
	//	moveVector += right * moveSpeed;
	//}
	//if (Input::GetKeyPress('Q')) // 上昇
	//{
	//	moveVector.y += moveSpeed;
	//}
	//if (Input::GetKeyPress('E')) // 下降
	//{
	//	moveVector.y -= moveSpeed;
	//}

    // カメラの位置を更新
	if (auto player = Game::GetInstance().FindObject<Player>().lock())
	{
		player->SetMouseCaptured(m_MouseCaptured);// プレイヤーのマウスキャプチャ状態をカメラと同期
		m_Position = player->GetPosition() + m_CameraOffset;// カメラの位置をプレイヤーの位置にオフセットを加えた位置に設定
		m_Target = (player->GetPosition() + player->GetForward());
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