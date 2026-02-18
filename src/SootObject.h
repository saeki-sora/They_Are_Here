#pragma once
#include "VisualObject.h"

class SootObject : public VisualObject
{
public:
	// コンストラクタ
	SootObject(
		const DirectX::SimpleMath::Vector3& pos = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f),
		const DirectX::SimpleMath::Vector3& size = DirectX::SimpleMath::Vector3(1.0f, 1.0f, 1.0f)
	);
	~SootObject() = default;

	void Init() override;
	void Update(float deltaTime) override;
	void Draw() override;

	// アニメーション設定
	void StartAnim(float duration);
	bool IsFinished() const { return m_Timer >= 1.0f; }

private:
	float m_Timer = 0.0f;
	float m_Duration = 1.0f;
	bool m_IsPlaying = false;

	// 専用定数バッファ
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_ConstantBuffer;
};