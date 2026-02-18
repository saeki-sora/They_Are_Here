#pragma once
#include "Effect.h"
#include "VisualObject.h"

// シェーダー用定数バッファ (Player.hにあったものを移動)
struct FoundConstantBuffer
{
    float time;
    float intensity;
    float padding[2];
};

class FoundEffect : public Effect
{
private:

    VisualObject m_ScreenPoly;      // 画面を覆う板
    ID3D11Buffer* m_ConstantBuffer; // 定数バッファ

    float m_Timer;       // 経過時間
    float m_Intensity;   // エフェクトの強さ
    float m_Duration;    // エフェクトが消えるまでの時間

public:

    FoundEffect();
    ~FoundEffect() override;

    void Update(float deltaTime) override;
    void Draw() override;

    bool IsPlaying() const override;

	// エフェクト再生中に、他の更新処理をブロックするか。trueでブロック
    bool ShouldBlockUpdate() const override { return false; }
};