#include "pch.h"
#include "Renderer.h"
#include "Camera.h"
#include "Application.h"
#include"input.h"
#include"SceneManager.h"
#include "Player.h"


using namespace DirectX::SimpleMath;

//=======================================
//初期化
//=======================================
void Camera::Init()
{
	m_Position = Vector3(0.0f, 40.0f, -50.0f);
	m_Target = Vector3(0.0f, 0.0f, 0.0f);
	m_Yaw = 0.0f;        // 水平回転角度
	m_Pitch = 0.0f;      // 垂直回転角度
	m_MouseSensitivity = 0.005f;  // マウス感度
	m_MouseCaptured = true;       // 初期状態はマウスキャプチャON
	m_CameraOffset = Vector3(0.0f, 10.0f, 0.0f); // カメラのオフセット


	// デフォルトのカーソルを保存
	m_DefaultCursor = LoadCursor(NULL, IDC_ARROW);

	// マウスカーソルを非表示にする
	while (ShowCursor(FALSE) >= 0);

    m_JustCaptured = true;   // 最初のフレーム回転を無効化
    CenterCursorToWindow();  // 中央にカーソルを移動

}

//=======================================
//更新処理
//=======================================
void Camera::Update(float deltaTime)
{
    // マウスキャプチャがOFFの時、左クリックでマウスキャプチャを有効化
    // ポーズ中（m_ClickToRecapture == false）はクリックしても復帰させない
    if (!m_MouseCaptured && m_ClickToRecapture && Input::GetKeyPress(VK_LBUTTON))
    {
        m_MouseCaptured = true;
        m_JustCaptured = true;     // 次フレームの回転を無効化
        while (ShowCursor(FALSE) >= 0);
        SetCursor(NULL);
		CenterCursorToWindow();// ウィンドウ中央にカーソルを移動
		Input::ResetMouseTracking();// GetMouseMove側の基準点も中央に同期させる
    }



    // =================================================================
    // モードごとの挙動
    // =================================================================
	// プレイヤー追従モード
    if (m_CurrentMode == Mode::FollowPlayer)
    {
        //プレイヤーを取得して動けるかどうかを確認
        std::shared_ptr<Player> player;
        {
            auto wp = SceneManager::GetInstance().FindObject<Player>();
            if (!wp.expired())
            {
                player = wp.lock();
            }
        }

        bool playerCanMove = true;
        if (player)
        {
            playerCanMove = player->CanMove();
        }



        if (player)
        {
            // プレイヤーのマウスキャプチャ状態をカメラと同期
            player->SetMouseCaptured(m_MouseCaptured);

            Vector3 eyeOffset = Vector3(0.0f, 8.0f, 0.0f);
            m_Position = player->GetPosition() + eyeOffset;

#ifndef _DEBUG

            Vector3 bobOffset = Vector3::Zero;

            //移動判定があるとき
            bool isMoving = (Input::GetKeyPress('W') || Input::GetKeyPress('A') ||
                Input::GetKeyPress('S') || Input::GetKeyPress('D'));

            if (isMoving && player->CanMove()) // プレイヤーが動ける状態かつ入力があるとき
            {
                // タイマーを進める
                m_WalkTimer += deltaTime * m_BobbingSpeed;

                float wave = sin(m_WalkTimer);
                bobOffset.y = wave * m_BobbingAmount;
            }
            else
            {
                // 止まっている時はタイマーを戻す
                m_WalkTimer = 0.0f;
                bobOffset.y = 0.0f;
            }

            // 最終的なカメラ位置
            // プレイヤー位置 + 基本オフセット + 揺れ
            Vector3 targetPos = player->GetPosition() + eyeOffset + bobOffset;

            //線形補間を使って滑らかに追従させる
            float smoothSpeed = 15.0f * deltaTime;

            m_Position = Vector3::Lerp(m_Position, targetPos, smoothSpeed);

#endif

            float playerYaw = player->GetYawRotation();
            float playerPitch = player->GetPitch();

            Vector3 forward;
            forward.x = sin(playerYaw) * cos(playerPitch);
            forward.y = sin(playerPitch);
            forward.z = cos(playerYaw) * cos(playerPitch);
            forward.Normalize();

            float lookDistance = 100.0f;
            m_Target = m_Position + (forward * lookDistance);
        }
    }


    if (m_CurrentMode == Mode::Free)
    {
        // ---------------------------------------------------
        // フリーカメラモード(チュートリアル用)
        // ---------------------------------------------------
        if (m_MouseCaptured)
        {
            // キャプチャON直後は1フレームぶれを殺す
            if (m_JustCaptured)
            {
                m_JustCaptured = false;
                CenterCursorToWindow();
            }
            else
            {
                POINT mousePos;
                GetCursorPos(&mousePos); // スクリーン座標

                POINT center = GetWindowCenterScreen(); // スクリーン座標の中央

                float deltaX = static_cast<float>(mousePos.x - center.x) * m_MouseSensitivity;
                float deltaY = static_cast<float>(mousePos.y - center.y) * m_MouseSensitivity;

                m_Yaw += deltaX;
                m_Pitch -= deltaY;

                m_Pitch = std::max(-1.5f, std::min(1.5f, m_Pitch));

                // 毎フレーム中央に戻す
                SetCursorPos(center.x, center.y);
            }
        }


        //カメラの方向ベクトル計算
        Vector3 forward;
        forward.x = sin(m_Yaw) * cos(m_Pitch);
        forward.y = sin(m_Pitch);
        forward.z = cos(m_Yaw) * cos(m_Pitch);
        forward.Normalize();

        Vector3 right; // 右方向ベクトル
        Vector3 up = Vector3(0, 1, 0);
        forward.Cross(up, right);
        right.Normalize();

        //キーボード移動
        float speed = m_FreeMoveSpeed * deltaTime;
        if (Input::GetKeyPress(VK_SHIFT)) speed *= 2.0f; // Shiftで高速移動

        if (Input::GetKeyPress('W')) m_Position += forward * speed;
        if (Input::GetKeyPress('S')) m_Position -= forward * speed;
        if (Input::GetKeyPress('D')) m_Position += right * speed;
        if (Input::GetKeyPress('A')) m_Position -= right * speed;
        if (Input::GetKeyPress(VK_SPACE)) m_Position.y += speed; // 上移動
        if (Input::GetKeyPress(VK_CONTROL)) m_Position.y -= speed; // 下移動

        //ターゲットの更新
        // 自分の位置から、向いている方向に少し先をターゲットにする
        m_Target = m_Position + forward * 10.0f;
    }

	// =================================================================
	//SetLookAt以外では動かせない
	// =================================================================
    if (m_CurrentMode == Mode::Stop)
    {
        // 何もしない
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
		// ビュー変換後の行列作成
		Vector3 up = Vector3(0.0f, 1.0f, 0.0f);
		m_ViewMatrix = DirectX::XMMatrixLookAtLH(m_Position, m_Target, up);

		Renderer::SetViewMatrix(&m_ViewMatrix);

		//プロジェクション行列の生成
		constexpr float fieldOfView = DirectX::XMConvertToRadians(90.0f);    // 視野角

		float aspectRatio = static_cast<float>(Application::GetWidth()) / static_cast<float>(Application::GetHeight());	// アスペクト比
		float nearPlane = 0.1f;       // ニアクリップ
		float farPlane = 10000.0f;      // ファークリップ

		//プロジェクション行列の生成
		m_ProjectionMatrix = DirectX::XMMatrixPerspectiveFovLH(fieldOfView, aspectRatio, nearPlane, farPlane);

		Renderer::SetProjectionMatrix(&m_ProjectionMatrix);

        //フラスタムを作成
        m_Frustum.CreateFromMatrix(m_Frustum, m_ProjectionMatrix);

        //ビュー行列の逆行列を取得
        Matrix invView = m_ViewMatrix.Invert();

        //フラスタムをワールド座標系に変換
        m_Frustum.Transform(m_Frustum, invView);

	}
	else if (mode == 1)//2D
	{
		Vector3 pos = { 0.0f,0.0f,-10.0f };
		Vector3 tgt = { 0.0f,0.0f,1.0f };
		Vector3 up = Vector3(0.0f, 1.0f, 0.0f);
		m_ViewMatrix = DirectX::XMMatrixLookAtLH(pos, tgt, up);
		Renderer::SetViewMatrix(&m_ViewMatrix);

		//プロジェクション行列の生成
		float nearPlane = 1.0f;       // ニアクリップ
		float farPlane = 10000.0f;      // ファークリップ
		Matrix projectionMatrix = DirectX::XMMatrixOrthographicLH(static_cast<float>(Application::GetWidth()),
			static_cast<float>(Application::GetHeight()), nearPlane, farPlane);

		projectionMatrix = DirectX::XMMatrixTranspose(projectionMatrix);
		Renderer::SetProjectionMatrix(&projectionMatrix);
	}
}


void Camera::SetLookAt(const Vector3& pos, const Vector3& target)
{
    m_Position = pos;
    m_Target = target;

    // カメラからターゲットへのベクトルを計算
    Vector3 forward = m_Target - m_Position;
    forward.Normalize(); // 長さを1にする

	m_Pitch = asin(forward.y);// ベクトルから角度(Pitch)を取得
	m_Yaw = atan2(forward.x, forward.z);// ベクトルから角度(Yaw)を取得
}



POINT Camera::GetWindowCenterScreen() const
{
    HWND hWnd = Application::GetWindow();

    // クライアント座標(0,0)を基準にスクリーン座標へ
    POINT center{};
    center.x = static_cast<LONG>(Application::GetWidth() / 2);
    center.y = static_cast<LONG>(Application::GetHeight() / 2);

    ClientToScreen(hWnd, &center);
    return center;
}



// カーソルをウィンドウ中央に移動
void Camera::CenterCursorToWindow()
{
    POINT c = GetWindowCenterScreen();
    SetCursorPos(c.x, c.y);
}


// ポーズ解除時に即マウス再キャプチャ
void Camera::RecaptureMouseImmediate()
{
    m_MouseCaptured = true;
    m_JustCaptured  = true;
    while (ShowCursor(FALSE) >= 0);
    SetCursor(NULL);
    CenterCursorToWindow();
    Input::ResetMouseTracking(); // GetMouseMove側の基準点も中央に同期させる
}


// ポーズ開始時にマウスを解放
void Camera::ReleaseMouseImmediate()
{
    m_MouseCaptured = false;
    while (ShowCursor(TRUE) < 0);
    SetCursor(m_DefaultCursor);
}

