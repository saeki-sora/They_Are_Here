#pragma once
#include "Object.h"
#include "MeshRenderer.h"
#include "Texture.h"
#include "Material.h"
#include"ColliderObject.h"


using Vector3 = DirectX::SimpleMath::Vector3;
class MakeMap;
class Block;
class DashEffect;

class Player :public ColliderObject
{
private:

	float m_Pitch; // 垂直回転角度（ピッチ）
	Vector3 m_Forward; // 前方ベクトル

	bool m_CanMove = true; // 移動可能フラグ

	float m_MoveSpeed; // 移動速度
	float m_MouseSensitivity;  // マウス感度
	bool m_MouseCaptured = true; // マウスキャプチャ状態

	float m_InvisibleTime;//透明化効果時間
	int m_MaxInvisibleStock = 3;//透明化アイテムの最大ストック数（難易度に応じて SetMaxInvisibleStock() で変更する）
	int m_InvisibleStock = 3;//透明化アイテムのストック数
	bool m_IsInvisible= false; //透明化フラグ
	float m_InvisibleTimer = 0.0f; //透明化タイマー

	static constexpr float WALL_BREAK_RANGE = 70.0f; // 壁を破壊できる距離
	static constexpr float WALL_BREAK_CHARGE_TIME = 1.0f; // 壁破壊に必要なチャージ時間(秒)
	static constexpr float MAX_YAW_OFFSET = DirectX::XM_PIDIV4; // チャージ中の最大左右回転角度(45度)
	static constexpr float MAX_PITCH_OFFSET = DirectX::XM_PI / 6.0f; // チャージ中の最大上下回転角度(30度)

	bool m_IsChargingBreak = false; // 壁破壊チャージ中フラグ
	float m_BreakChargeTimer = 0.0f; // 壁破壊チャージタイマー
	float m_ChargeStartYaw = 0.0f; // チャージ開始時のYaw角度
	float m_ChargeStartPitch = 0.0f; // チャージ開始時のPitch角度
	bool m_WallBreakJustCompleted = false; // 壁破壊が完了した直後のフラグ
	std::weak_ptr<Block> m_TargetBlock; // チャージ中の破壊対象ブロック

	bool m_HasKey = false;//カギを持っているかどうか
	float m_NoKeyMessageTimer = 0.0f;//カギを持っていないときに出す画像の表示時間

	bool m_IsGoal = false; //ゴールフラグ

	MakeMap* m_Map = nullptr; // マップグリッド操作用

	// 壁ブロックリストキャッシュ
	mutable std::vector<std::weak_ptr<Block>> m_CachedBlocks;

	// スタミナ関連
	float m_Stamina     = 100.0f;  // スタミナ現在値
	float m_MaxStamina  = 100.0f;  // スタミナ最大値
	bool  m_IsDashing   = false;   // ダッシュ中フラグ
	bool  m_IsExhausted = false;   // スタミナ枯渇ペナルティ中
	float m_ExhaustedTimer = 0.0f; // 枯渇ペナルティ残り時間
	bool  m_DashLocked  = false;   // 枯渇後に Shift 再押しが必要

	static constexpr float DASH_SPEED_MULT       = 1.8f;  // ダッシュ時の速度倍率
	static constexpr float STAMINA_DRAIN_RATE    = 30.0f; // 消費速度 (/秒)
	static constexpr float STAMINA_REGEN_NEAR    =  5.0f; // 赤ゾーン回復速度
	static constexpr float STAMINA_REGEN_MID     = 12.0f; // 青ゾーン回復速度
	static constexpr float STAMINA_REGEN_FAR     = 20.0f; // 緑ゾーン回復速度
	static constexpr float STAMINA_DASH_THRESHOLD = 15.0f; // ダッシュ開始に必要な最低スタミナ
	static constexpr float EXHAUSTED_COOLDOWN    =  2.0f; // 枯渇後のダッシュ禁止時間（秒）

	DashEffect* m_DashEffect = nullptr; // ダッシュエフェクト（所有権はEffectManager）

	// Update を責務ごとに分割した内部ヘルパー
	void UpdateWallBreak(float deltaTime);
	void UpdateLook();
	void UpdateDashAndStamina(float deltaTime);
	void UpdateMovement(float deltaTime);
	void UpdateStaminaRegen(float deltaTime);
	void UpdateGoalCheck();

public:

	Player(
		const DirectX::SimpleMath::Vector3& pos,
		const DirectX::SimpleMath::Vector3& size);//コンストラクタ

	~Player(); // デストラクタ

	void Init()override;
	void Update(float deltaTime)override;
	void Draw()override;
	void Uninit()override;

	bool CheckCollisionWithBlocks(const Vector3& newPosition);// ブロックとの衝突判定

	
	void StartInvisible(float duration);//透明化を開始する
	bool IsInvisible() const;//透明化中か否か
	void AddInvisibleStock(int amount);//ストックを増やす関数

	void TryBreakWall(); // 壁破壊を試みる
	std::weak_ptr<Block> GetBlockInFront() const; // 前方のブロックを取得
	bool IsChargingBreak() const; // チャージ中か確認
	float GetBreakChargeProgress() const; // チャージ進行度を取得(0.0~1.0)

	//セッター
	void SetMap(MakeMap* map);
	void SetPosition(const DirectX::SimpleMath::Vector3& pos);
	void SetRotation(const Vector3& rotation);
	void SetYawRotation(float yaw);
	void SetPitch(float pitch);
	void SetMouseSensitivity(float sensitivity);// マウス感度の設定
	void SetMouseCaptured(bool captured);// マウスキャプチャ状態の設定
	void SetCanMove(bool canMove);// 移動可能フラグの設定
	void SetHasKey(bool hasKey);// カギ所持フラグの設定
	void SetMaxInvisibleStock(int stock);// 最大ストック数を設定し、現在ストックも合わせる


	//ゲッター
	Vector3 GetPosition() const override; // ポジションの取得
	Vector3 GetRotation() const;// 回転の取得
	float GetYawRotation() const; // 水平回転角度（ヨー）の取得
	float GetPitch() const;// 垂直回転角度（ピッチ）の取得
	Vector3 GetForward() const; // 前方ベクトルの取得
	float GetMouseSensitivity() const;
	bool IsMouseCaptured() const;
	bool GetGoalFlag() const;// ゴールフラグの取得
	bool CanMove() const;// 移動可能フラグの取得
	bool HasKey() const;// カギ所持フラグの取得
	int GetInvisibleStock() const;//現在の透明化アイテムの数の取得
	int GetMaxInvisibleStock() const;//透明化アイテムの最大数の取得

	// スタミナ関連ゲッター
	float GetStamina() const;
	float GetMaxStamina() const;
	bool  IsDashing() const;
};
