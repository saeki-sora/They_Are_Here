#pragma once
#include "SceneTransition.h"
#include "Shader.h"  

struct ID3D11Buffer;

class FadeTransition : public SceneTransition 
{
private:

	static Shader m_Shader;
	static ComPtr<ID3D11Buffer> m_VertexBuffer;
	static ComPtr<ID3D11Buffer> m_ConstantBuffer;

	Phase m_Phase = Phase::FadeOut;
	float m_Duration = 1.0f;
	float m_Timer = 0.0f;
	DirectX::SimpleMath::Color m_FadeColor{ 0, 0, 0, 1 };
	bool m_IsComplete = false;

	void InitResources();

public:
	FadeTransition(float duration = 1.0f,
		const DirectX::SimpleMath::Color& color = DirectX::SimpleMath::Color(0, 0, 0, 1));

	void Start() override;
	void Update(float deltaTime) override;
	void Draw() override;

	bool IsComplete() const override { return m_IsComplete; }
	Phase GetPhase() const override { return m_Phase; }
	float GetProgress() const override;

	void SetPhase(Phase phase) { m_Phase = phase; m_Timer = 0.0f; }
};
