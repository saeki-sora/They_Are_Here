#include "pch.h"
#include "input.h"

BYTE Input::keyState[256] = {};
BYTE Input::keyState_old[256] = {};
XINPUT_STATE Input::controllerState = {};
XINPUT_STATE Input::controllerState_old = {};
int Input::VibrationTime = 0;

static int g_LastMouseX = 0;
static int g_LastMouseY = 0;
static bool g_FirstMouse = true;

//コンストラクタ
Input::Input()
{
	
}

//デストラクタ
Input::~Input()
{
	//振動を終了させる
	XINPUT_VIBRATION vibration;
	ZeroMemory(&vibration, sizeof(XINPUT_VIBRATION));
	vibration.wLeftMotorSpeed = 0;
	vibration.wRightMotorSpeed = 0;
	XInputSetState(0, &vibration);
}

void Input::Update()
{
	//1フレーム前の入力を記録しておく
	for (int i = 0; i < 256; i++) { keyState_old[i] = keyState[i]; }
	controllerState_old = controllerState;

	//キー入力を更新
	BOOL hr = GetKeyboardState(keyState);

	//コントローラー入力を更新(XInput)
	XInputGetState(0, &controllerState);

	//振動継続時間をカウント
	if (VibrationTime > 0) {
		VibrationTime--;
		if (VibrationTime == 0) { //振動継続時間が経った時に振動を止める
			XINPUT_VIBRATION vibration;
			ZeroMemory(&vibration, sizeof(XINPUT_VIBRATION));
			vibration.wLeftMotorSpeed = 0;
			vibration.wRightMotorSpeed = 0;
			XInputSetState(0, &vibration);
		}
	}
}

//キー入力
bool Input::GetKeyPress(int key) //プレス
{
	return keyState[key] & 0x80;
}
bool Input::GetKeyTrigger(int key) //トリガー
{
	return (keyState[key] & 0x80) && !(keyState_old[key] & 0x80);
}
bool Input::GetKeyRelease(int key) //リリース
{
	return !(keyState[key] & 0x80) && (keyState_old[key] & 0x80);
}

//左アナログスティック
DirectX::SimpleMath::Vector2 Input::GetLeftAnalogStick(void)
{
	SHORT x = controllerState.Gamepad.sThumbLX; // -32768～32767
	SHORT y = controllerState.Gamepad.sThumbLY; // -32768～32767

	DirectX::XMFLOAT2 res;
	res.x = x / 32767.0f; //-1～1
	res.y = y / 32767.0f; //-1～1
	return res;
}
//右アナログスティック
DirectX::SimpleMath::Vector2 Input::GetRightAnalogStick(void)
{
	SHORT x = controllerState.Gamepad.sThumbRX; // -32768～32767
	SHORT y = controllerState.Gamepad.sThumbRY; // -32768～32767

	DirectX::XMFLOAT2 res;
	res.x = x / 32767.0f; //-1～1
	res.y = y / 32767.0f; //-1～1
	return res;
}

//左トリガー
float Input::GetLeftTrigger(void)
{
	BYTE t = controllerState.Gamepad.bLeftTrigger; // 0～255
	return t / 255.0f;
}
//右トリガー
float Input::GetRightTrigger(void)
{
	BYTE t = controllerState.Gamepad.bRightTrigger; // 0～255
	return t / 255.0f;
}

//ボタン入力
bool Input::GetButtonPress(WORD btn) //プレス
{
	return (controllerState.Gamepad.wButtons & btn) != 0;
}
bool Input::GetButtonTrigger(WORD btn) //トリガー
{
	return (controllerState.Gamepad.wButtons & btn) != 0 && (controllerState_old.Gamepad.wButtons & btn) == 0;
}
bool Input::GetButtonRelease(WORD btn) //リリース
{
	return (controllerState.Gamepad.wButtons & btn) == 0 && (controllerState_old.Gamepad.wButtons & btn) != 0;
}

//振動
void Input::SetVibration(int frame, float powor)
{
	// XINPUT_VIBRATION構造体のインスタンスを作成
	XINPUT_VIBRATION vibration;
	ZeroMemory(&vibration, sizeof(XINPUT_VIBRATION));

	// モーターの強度を設定（0～65535）
	vibration.wLeftMotorSpeed = (WORD)(powor * 65535.0f);
	vibration.wRightMotorSpeed = (WORD)(powor * 65535.0f);
	XInputSetState(0, &vibration);

	//振動継続時間を代入
	VibrationTime = frame;
}

void Input::GetMouseMove(int* deltaX, int* deltaY)
{
	POINT currentPos;
	GetCursorPos(&currentPos);

	// スクリーン座標をクライアント座標に変換
	HWND hwnd = GetActiveWindow(); // または適切なウィンドウハンドルを取得
	ScreenToClient(hwnd, &currentPos);

	if (g_FirstMouse)
	{
		g_LastMouseX = currentPos.x;
		g_LastMouseY = currentPos.y;
		g_FirstMouse = false;
		*deltaX = 0;
		*deltaY = 0;
		return;
	}

	*deltaX = currentPos.x - g_LastMouseX;
	*deltaY = currentPos.y - g_LastMouseY;

	g_LastMouseX = currentPos.x;
	g_LastMouseY = currentPos.y;

	// マウスをウィンドウ中央に固定する
	// カーソルが非表示の時のみ実行
	CURSORINFO ci = { sizeof(CURSORINFO) };
	GetCursorInfo(&ci);
	if (ci.flags == 0) // カーソルが非表示の場合
	{
		RECT rect;
		GetClientRect(hwnd, &rect);
		int centerX = (rect.right - rect.left) / 2;
		int centerY = (rect.bottom - rect.top) / 2;
		POINT center = { centerX, centerY };
		ClientToScreen(hwnd, &center);
		SetCursorPos(center.x, center.y);
		g_LastMouseX = centerX;
		g_LastMouseY = centerY;
	}
}