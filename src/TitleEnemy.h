#pragma once
#include "Object.h"
#include "ColliderObject.h"

class TitleEnemy : public ColliderObject
{
public:

	// 敵の状態定義
	enum class State
	{
		Idle,       // 待機
		Turning,    // ゆっくり振り返り
		Dashing     // 猛ダッシュ
	};

	TitleEnemy(
		const DirectX::SimpleMath::Vector3& pos = { 0.0f, 0.0f, 0.0f },
		const DirectX::SimpleMath::Vector3& size = { 1.0f, 1.0f, 1.0f });
	~TitleEnemy();
	void Init() override;
	void Update(float deltaTime) override;
	void Draw() override;
	void Uninit() override;

	void StartScareSequence(const DirectX::SimpleMath::Vector3& cameraPos);

private:

	State m_State = State::Idle;

	DirectX::SimpleMath::Vector3 m_TargetPos; // 目標（カメラ）

	// 各ステート用変数
	float m_TurnSpeed = 2.0f;       // 振り返る速さ（小さいほど遅い）
	float m_DashSpeed = 80.0f;      // ダッシュ速度

	float m_Timer = 0.0f;           // 汎用タイマー

	// 揺れの設定
	float m_SwaySpeed = 0.0f;         // 左右に揺れる速さ
	float m_SwayAmount = 0.5f;         // 左右の揺れ幅
	float m_BobAmount = 0.0f;          // 上下の揺れ幅（足踏み感）
};

