// 最終合成 ピクセルシェーダー
// HDRシーンにブルーム・SSAO・ボリュメトリックを合成し、
// トーンマップ＋カラーグレード＋ビネット＋グレインを適用してLDRへ出力する
// 出力αには後段FXAA用の輝度（luma）を書き込む
#include "postCommon.hlsl"

Texture2D g_SceneTex : register(t0);
Texture2D g_BloomTex0 : register(t2);
Texture2D g_BloomTex1 : register(t3);
Texture2D g_BloomTex2 : register(t4);
Texture2D g_SSAOTex : register(t5);
Texture2D g_VolumetricTex : register(t6);
SamplerState g_LinearClamp : register(s0);

float4 ps_main(in PS_IN_POST input) : SV_Target
{
    float3 color = g_SceneTex.Sample(g_LinearClamp, input.tex).rgb;

    // --- SSAO（環境遮蔽を乗算） ---
    if (PP_SSAOParams.w > 0.0f)
    {
        float ao = g_SSAOTex.Sample(g_LinearClamp, input.tex).r;
        color *= ao;
    }

    // --- ブルーム（3Mipの重み付き加算） ---
    if (PP_BloomParams.z > 0.0f)
    {
        float3 bloom = g_BloomTex0.Sample(g_LinearClamp, input.tex).rgb * PP_BloomWeights.x
                     + g_BloomTex1.Sample(g_LinearClamp, input.tex).rgb * PP_BloomWeights.y
                     + g_BloomTex2.Sample(g_LinearClamp, input.tex).rgb * PP_BloomWeights.z;
        color += bloom * PP_BloomParams.z;
    }

    // --- ボリュメトリックライト（加算） ---
    if (PP_VolumetricParams.w > 0.0f)
    {
        color += g_VolumetricTex.Sample(g_LinearClamp, input.tex).rgb;
    }

    // --- 露出 + ACESトーンマップ ---
    color *= PP_ToneParams.x;
    if (PP_ToneParams.y < 0.5f)
    {
        color = ACESFilm(color);
    }
    else
    {
        color = saturate(color);
    }

    // --- カラーグレード（ティント + 彩度調整） ---
    float luma = Luminance(color);
    color = lerp(luma.xxx, color, PP_ToneParams.z);
    color = lerp(color, color * PP_ColorTint.rgb, PP_ColorTint.w);

    // --- ビネット（画面端を暗く落とす） ---
    float2 centered = input.tex - 0.5f;
    float vignette = 1.0f - PP_VignetteParams.x * pow(saturate(dot(centered, centered) * 2.0f), PP_VignetteParams.y);
    color *= vignette;

    // --- フィルムグレイン（時間でシードを揺らす） ---
    float grain = InterleavedGradientNoise(input.pos.xy + frac(PP_ToneParams.w) * 1024.0f) - 0.5f;
    color += grain * PP_VignetteParams.z;

    color = saturate(color);

    // αにはFXAA用の輝度を書き込む
    return float4(color, Luminance(color));
}
