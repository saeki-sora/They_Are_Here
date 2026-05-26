#pragma once
#include "Effect.h"
#include "Shader.h"


// 前方宣言
struct ID3D11VertexShader;
struct ID3D11PixelShader;
struct ID3D11InputLayout;
struct ID3D11Buffer;


class FadeEffect : public Effect
{
public:

	// duration: フェードにかかる時間(秒), isFadeIn: trueならフェードイン、falseならフェードアウト
	FadeEffect(float duration, bool isFadeIn);

	virtual ~FadeEffect()
	{
		// デストラクタが呼ばれたことを出力
#ifdef _DEBUG
		std::cout << "[FadeEffect] Destructor called. Address: " << this << std::endl;
#endif
	}

	void Update(float deltaTime) override;
	void Draw() override;
	bool IsPlaying() const override { return m_isPlaying; }

private:
	bool m_isFadeIn;//true:フェードイン false:フェードアウト
	bool m_isPlaying = true;
	float m_timer = 0.0f;
	float m_duration;// フェードの総時間

	// 描画リソース (Shaderクラスの代わりにD3D11オブジェクトを直接保持)
	static Shader m_shader;
	static ComPtr<ID3D11Buffer> m_vertexBuffer;
	static ComPtr<ID3D11Buffer> m_constantBuffer;

	// リソースを初期化
	void InitResources();
};