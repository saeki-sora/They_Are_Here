// ブルーム用 輝度抽出 ピクセルシェーダー
#include "postCommon.hlsl"

Texture2D g_SceneTex : register(t0);
SamplerState g_LinearClamp : register(s0);

float4 ps_main(in PS_IN_POST input) : SV_Target
{
    float3 color = g_SceneTex.Sample(g_LinearClamp, input.tex).rgb;

    // 異常値(Inf/負値)がブルームのぼかしで画面全体へ広がらないよう有限範囲へ丸める
    color = min(max(color, 0.0f), 65504.0f);

    // ソフトニー付きの閾値抽出（閾値付近を滑らかに立ち上げる）
    float threshold = PP_BloomParams.x;
    float knee = PP_BloomParams.y;
    float luma = Luminance(color);
    float soft = saturate((luma - threshold + knee) / (2.0f * knee));
    soft = soft * soft;
    float contribution = max(soft, step(threshold, luma));

    // 輝度に対する超過分だけを残す
    float scale = contribution * saturate((luma - threshold) / max(luma, 0.0001f));
    return float4(color * scale, 1.0f);
}
