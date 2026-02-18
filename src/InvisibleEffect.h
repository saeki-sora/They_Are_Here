#pragma once
#include "Effect.h"
#include "VisualObject.h"
#include "Renderer.h"

// 定数バッファ (InvisibleItemにあったものを移動)
struct InvisibleConstantBuffer
{
    float time;
    float totalTime;
    float padding[2];
};

class InvisibleEffect : public Effect
{
private:
    VisualObject m_ScreenPoly;      // 画面を覆う板
    ID3D11Buffer* m_ConstantBuffer; // 定数バッファ

    float m_Timer;       // 経過時間
    float m_Duration;    // 効果時間

public:
    // コンストラクタで持続時間を受け取る
    InvisibleEffect(float duration);
    ~InvisibleEffect() override;

    void Update(float deltaTime) override;
    void Draw() override;

    bool IsPlaying() const override;

    // ゲームを止めないエフェクトにする
    bool ShouldBlockUpdate() const override { return false; }
};