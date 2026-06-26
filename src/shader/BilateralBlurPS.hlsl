// 深度参照バイラテラルブラー ピクセルシェーダー（SSAOのノイズ除去用）
#include "postCommon.hlsl"

Texture2D g_SourceTex : register(t0);
Texture2D g_DepthTex : register(t1);
SamplerState g_LinearClamp : register(s0);
SamplerState g_PointClamp : register(s1);

float4 ps_main(in PS_IN_POST input) : SV_Target
{
    static const float WEIGHTS[5] = { 0.2270270f, 0.1945946f, 0.1216216f, 0.0540541f, 0.0162162f };

    float2 stepUV = PP_BlurDir.xy * PP_ScreenInfo.zw;
    float centerDepth = LinearizeDepth(g_DepthTex.SampleLevel(g_PointClamp, input.tex, 0).r);

    float result = g_SourceTex.Sample(g_LinearClamp, input.tex).r * WEIGHTS[0];
    float totalWeight = WEIGHTS[0];

    [unroll]
    for (int i = 1; i < 5; ++i)
    {
        float2 offset = stepUV * i;

        [unroll]
        for (int s = -1; s <= 1; s += 2)
        {
            float2 uv = input.tex + offset * s;
            float sampleDepth = LinearizeDepth(g_DepthTex.SampleLevel(g_PointClamp, uv, 0).r);

            // 深度差が大きいサンプルは別の面とみなし重みを落とす
            float depthWeight = saturate(1.0f - abs(sampleDepth - centerDepth) / (centerDepth * 0.1f + 1.0f));
            float w = WEIGHTS[i] * depthWeight;

            result += g_SourceTex.Sample(g_LinearClamp, uv).r * w;
            totalWeight += w;
        }
    }

    float ao = result / max(totalWeight, 0.0001f);
    return float4(ao.xxx, 1.0f);
}
