// ダウンサンプル ピクセルシェーダー（バイリニア1タップ＝2x2ボックス平均）
#include "postCommon.hlsl"

Texture2D g_SourceTex : register(t0);
SamplerState g_LinearClamp : register(s0);

float4 ps_main(in PS_IN_POST input) : SV_Target
{
    return g_SourceTex.Sample(g_LinearClamp, input.tex);
}
