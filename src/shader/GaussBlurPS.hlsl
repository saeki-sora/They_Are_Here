// 分離型ガウスブラー ピクセルシェーダー（方向は PP_BlurDir で指定）
#include "postCommon.hlsl"

Texture2D g_SourceTex : register(t0);
SamplerState g_LinearClamp : register(s0);

float4 ps_main(in PS_IN_POST input) : SV_Target
{
    // 9タップ分のガウス重み（σ≒2.0）
    static const float WEIGHTS[5] = { 0.2270270f, 0.1945946f, 0.1216216f, 0.0540541f, 0.0162162f };

    float2 stepUV = PP_BlurDir.xy * PP_ScreenInfo.zw;

    float3 result = g_SourceTex.Sample(g_LinearClamp, input.tex).rgb * WEIGHTS[0];

    [unroll]
    for (int i = 1; i < 5; ++i)
    {
        float2 offset = stepUV * i;
        result += g_SourceTex.Sample(g_LinearClamp, input.tex + offset).rgb * WEIGHTS[i];
        result += g_SourceTex.Sample(g_LinearClamp, input.tex - offset).rgb * WEIGHTS[i];
    }

    return float4(result, 1.0f);
}
