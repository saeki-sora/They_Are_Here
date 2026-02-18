#pragma once
#include "Effect.h"
#include "VisualObject.h"
#include "Renderer.h"

// ダッシュ時の定数バッファ
struct DashConstantBuffer
{
    float time;     // 累積時間（ライン流れ用）
    float alpha;    // 現在の透明度
    float padding[2];
};

class DashEffect : public Effect
{
private:
    VisualObject m_ScreenPoly;      // 画面を覆う板
    ID3D11Buffer* m_ConstantBuffer; // 定数バッファ

    float m_Timer = 0.0f;           // 累積時間（ライン流れ用）
    float m_Alpha = 0.0f;           // 現在のフェード値 (0→1)
    bool  m_Active = true;          // アクティブフラグ

    static constexpr float FADE_IN_SPEED  = 5.0f; // フェードイン速度
    static constexpr float FADE_OUT_SPEED = 3.0f; // フェードアウト速度

public:
    DashEffect();
    ~DashEffect() override;

    void Update(float deltaTime) override;
    void Draw() override;
    bool IsPlaying() const override;

    // ゲームを止めないエフェクト
    bool ShouldBlockUpdate() const override { return false; }

    // 外部から開始/停止を制御
    void SetActive(bool active) { m_Active = active; }
};
