#include "pch.h"
#include "Renderer.h"
#include "Camera.h"
#include "Application.h"
#include"input.h"
#include"SceneManager.h"
#include "Player.h"


using namespace DirectX::SimpleMath;

//=======================================
//初期化�E琁E
//=======================================
void Camera::Init()
{
	m_Position = Vector3(0.0f, 40.0f, -50.0f);
	m_Target = Vector3(0.0f, 0.0f, 0.0f);
	m_Yaw = 0.0f;        // 水平回転角度
	m_Pitch = 0.0f;      // 垂直回転角度
	m_MouseSensitivity = 0.005f;  // マウス感度
	m_MouseCaptured = true;       // 初期状態�EマウスキャプチャON
	m_EscKeyPressed = false;      // ESCキーフラグ初期匁E
	m_CameraOffset = Vector3(0.0f, 10.0f, 0.0f); // カメラのオフセチE��


	// チE��ォルトカーソルを保孁E
	m_DefaultCursor = LoadCursor(NULL, IDC_ARROW);

	// マウスカーソルを非表示にする
	while (ShowCursor(FALSE) >= 0);

    m_JustCaptured = true;   // 最初�E1フレーム回転を無効匁E
    CenterCursorToWindow();  // 中忁E��寁E��めE

}

//=======================================
//更新処琁E
//=======================================
void Camera::Update(float deltaTime)
{

    // ESCキーでマウスキャプチャの刁E��替えと終亁E��誁E
    bool escKeyCurrentlyPressed = Input::GetKeyPress(VK_ESCAPE);
    if (escKeyCurrentlyPressed && !m_EscKeyPressed)
    {
        if (m_MouseCaptured)
        {
            // マウスキャプチャを解除
            m_MouseCaptured = false;
            while (ShowCursor(TRUE) < 0);
            SetCursor(m_DefaultCursor);
        }

        HWND hWnd = Application::GetWindow();
        if (hWnd != nullptr)
        {
            PostMessage(hWnd, WM_CLOSE, 0, 0);
        }
    }
    m_EscKeyPressed = escKeyCurrentlyPressed;



    // マウスキャプチャがOFFの時、左クリチE��でマウスキャプチャを�E閁E
    if (!m_MouseCaptured && Input::GetKeyPress(VK_LBUTTON))
    {
        m_MouseCaptured = true;
        m_JustCaptured = true;     // 次フレームの回転を無効匁E
        while (ShowCursor(FALSE) >= 0);
        SetCursor(NULL);
		CenterCursorToWindow();// ウィンドウ中央にカーソルを移勁E
    }



    // =================================================================
    // モードごとの挙動
    // =================================================================
	// プレイヤー追従モーチE
    if (m_CurrentMode == Mode::FollowPlayer)
    {
        //プレイヤーを取得して動けるかどぁE��を確誁E
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

            //移動判定があるとぁE
            bool isMoving = (Input::GetKeyPress('W') || Input::GetKeyPress('A') ||
                Input::GetKeyPress('S') || Input::GetKeyPress('D'));

            if (isMoving && player->CanMove()) // プレイヤーが動ける状態かつ入力がある晁E
            {
                // タイマ�Eを進める
                m_WalkTimer += deltaTime * m_BobbingSpeed;

                float wave = sin(m_WalkTimer);
                bobOffset.y = wave * m_BobbingAmount;
            }
            else
            {
                // 止まってぁE��時�Eタイマ�EめEに戻ぁE
                m_WalkTimer = 0.0f;
                bobOffset.y = 0.0f;
            }

            // 最終的なカメラ位置
            // プレイヤー位置 �E�E基本オフセチE�� �E�E揺めE
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
        // フリーカメラモーチE(チE��チE��用)
        // ---------------------------------------------------
        if (m_MouseCaptured)
        {
            // キャプチャON直後�E1フレームぶれを殺ぁE
            if (m_JustCaptured)
            {
                m_JustCaptured = false;
                CenterCursorToWindow();
            }
            else
            {
                POINT mousePos;
                GetCursorPos(&mousePos); // スクリーン座樁E

                POINT center = GetWindowCenterScreen(); // スクリーン座標�E中忁E

                float deltaX = static_cast<float>(mousePos.x - center.x) * m_MouseSensitivity;
                float deltaY = static_cast<float>(mousePos.y - center.y) * m_MouseSensitivity;

                m_Yaw += deltaX;
                m_Pitch -= deltaY;

                m_Pitch = std::max(-1.5f, std::min(1.5f, m_Pitch));

                // 毎フレーム中忁E��戻ぁE
                SetCursorPos(center.x, center.y);
            }
        }


        //カメラの方向�Eクトル計箁E
        Vector3 forward;
        forward.x = sin(m_Yaw) * cos(m_Pitch);
        forward.y = sin(m_Pitch);
        forward.z = cos(m_Yaw) * cos(m_Pitch);
        forward.Normalize();

        Vector3 right; // 右方吁E
        Vector3 up = Vector3(0, 1, 0);
        forward.Cross(up, right);
        right.Normalize();

        //キーボ�Eド移勁E
        float speed = m_FreeMoveSpeed * deltaTime;
        if (Input::GetKeyPress(VK_SHIFT)) speed *= 2.0f; // Shiftで高速移勁E

        if (Input::GetKeyPress('W')) m_Position += forward * speed;
        if (Input::GetKeyPress('S')) m_Position -= forward * speed;
        if (Input::GetKeyPress('D')) m_Position += right * speed;
        if (Input::GetKeyPress('A')) m_Position -= right * speed;
        if (Input::GetKeyPress(VK_SPACE)) m_Position.y += speed; // 上�E
        if (Input::GetKeyPress(VK_CONTROL)) m_Position.y -= speed; // 下降

        //ターゲチE��の更新
        // 自刁E�E位置から、向ぁE��ぁE��方向�E少し先をターゲチE��にする
        m_Target = m_Position + forward * 10.0f;


        //チE��チE��ログ出劁E(1秒ごと)
        m_DebugLogTimer += deltaTime;
        if (m_DebugLogTimer >= 1.0f)
        {
            m_DebugLogTimer = 0.0f; // タイマ�EリセチE��

            // コンソール出劁E
            printf("[DebugCamera] Pos(%.1f, %.1f, %.1f) Target(%.1f, %.1f, %.1f) Yaw:%.2f Pitch:%.2f\n",
                m_Position.x, m_Position.y, m_Position.z,
                m_Target.x, m_Target.y, m_Target.z,
                m_Yaw, m_Pitch);
        }
    }

	// =================================================================
	//SetLookAt以外では動かせなぁE��ーチE
	// =================================================================
    if (m_CurrentMode == Mode::Stop)
    {
        // 何もしなぁE
	}

}


//=======================================
//描画処琁E
//=======================================
void Camera::Draw()
{

}

//=======================================
//終亁E�E琁E
//=======================================
void Camera::Uninit() {}

//=======================================
//カメラの設宁E
//=======================================
void Camera::SetCamera(int mode)
{
	//3D
	if (mode == 0)
	{
		// ビュー変換後�E作�E
		Vector3 up = Vector3(0.0f, 1.0f, 0.0f);
		m_ViewMatrix = DirectX::XMMatrixLookAtLH(m_Position, m_Target, up);

		Renderer::SetViewMatrix(&m_ViewMatrix);

		//プロジェクション行�Eの生�E
		constexpr float fieldOfView = DirectX::XMConvertToRadians(90.0f);    // 視野见E

		float aspectRatio = static_cast<float>(Application::GetWidth()) / static_cast<float>(Application::GetHeight());	// アスペクト毁E
		float nearPlane = 0.1f;       // ニアクリチE�E
		float farPlane = 10000.0f;      // ファークリチE�E

		//プロジェクション行�Eの生�E
		m_ProjectionMatrix = DirectX::XMMatrixPerspectiveFovLH(fieldOfView, aspectRatio, nearPlane, farPlane);

		Renderer::SetProjectionMatrix(&m_ProjectionMatrix);

        //封E��行�Eから基本のフラスタムを作�E
        m_Frustum.CreateFromMatrix(m_Frustum, m_ProjectionMatrix);

        //ビュー行�Eの送E���Eを取征E
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

		//プロジェクション行�Eの生�E
		float nearPlane = 1.0f;       // ニアクリチE�E
		float farPlane = 10000.0f;      // ファークリチE�E
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

    // カメラからターゲチE��へのベクトルを計箁E
    Vector3 forward = m_Target - m_Position;
    forward.Normalize(); // 長さを1にする

	m_Pitch = asin(forward.y);// ベクトルから角度(Pitch)を送E��E
	m_Yaw = atan2(forward.x, forward.z);// ベクトルから角度(Yaw)を送E��E
}



POINT Camera::GetWindowCenterScreen() const
{
    HWND hWnd = Application::GetWindow();

    // クライアント中忁E0,0基溁E ↁEスクリーン座標へ
    POINT center{};
    center.x = static_cast<LONG>(Application::GetWidth() / 2);
    center.y = static_cast<LONG>(Application::GetHeight() / 2);

    ClientToScreen(hWnd, &center);
    return center;
}


void Camera::CenterCursorToWindow()
{
    POINT c = GetWindowCenterScreen();
    SetCursorPos(c.x, c.y);
}

