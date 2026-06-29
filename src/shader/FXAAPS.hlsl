// FXAA ピクセルシェーダー（簡略版FXAA 3.11 Quality。前段Compositeがαに書いたlumaを使う）
#include "postCommon.hlsl"

Texture2D g_SourceTex : register(t0);
SamplerState g_LinearClamp : register(s0);

// エッジ検出の閾値
static const float EDGE_THRESHOLD_MIN = 0.0312f;
static const float EDGE_THRESHOLD_MAX = 0.125f;
// エッジ探索の最大ステップ数
static const int SEARCH_STEPS = 12;
// サブピクセルAAの強さ
static const float SUBPIXEL_QUALITY = 0.75f;

float SampleLuma(float2 uv)
{
    return g_SourceTex.SampleLevel(g_LinearClamp, uv, 0).a;
}

float4 ps_main(in PS_IN_POST input) : SV_Target
{
    float2 invSize = PP_ScreenInfo.zw;
    float2 uv = input.tex;

    float3 colorCenter = g_SourceTex.SampleLevel(g_LinearClamp, uv, 0).rgb;

    // 十字方向のluma取得
    float lumaC = SampleLuma(uv);
    float lumaD = SampleLuma(uv + float2(0, invSize.y));
    float lumaU = SampleLuma(uv - float2(0, invSize.y));
    float lumaL = SampleLuma(uv - float2(invSize.x, 0));
    float lumaR = SampleLuma(uv + float2(invSize.x, 0));

    float lumaMin = min(lumaC, min(min(lumaD, lumaU), min(lumaL, lumaR)));
    float lumaMax = max(lumaC, max(max(lumaD, lumaU), max(lumaL, lumaR)));
    float lumaRange = lumaMax - lumaMin;

    // コントラストが低ければAA不要
    if (lumaRange < max(EDGE_THRESHOLD_MIN, lumaMax * EDGE_THRESHOLD_MAX))
    {
        return float4(colorCenter, 1.0f);
    }

    // 斜め方向のluma取得
    float lumaDL = SampleLuma(uv + float2(-invSize.x, invSize.y));
    float lumaDR = SampleLuma(uv + float2(invSize.x, invSize.y));
    float lumaUL = SampleLuma(uv - float2(invSize.x, invSize.y));
    float lumaUR = SampleLuma(uv - float2(-invSize.x, invSize.y));

    // エッジの方向（水平/垂直）を判定する
    float edgeHorz = abs(lumaUL + lumaUR - 2.0f * lumaU) + 2.0f * abs(lumaL + lumaR - 2.0f * lumaC) + abs(lumaDL + lumaDR - 2.0f * lumaD);
    float edgeVert = abs(lumaUL + lumaDL - 2.0f * lumaL) + 2.0f * abs(lumaU + lumaD - 2.0f * lumaC) + abs(lumaUR + lumaDR - 2.0f * lumaR);
    bool isHorizontal = (edgeHorz >= edgeVert);

    // エッジと直交する方向の勾配からエッジ側を決める
    float luma1 = isHorizontal ? lumaU : lumaL;
    float luma2 = isHorizontal ? lumaD : lumaR;
    float gradient1 = luma1 - lumaC;
    float gradient2 = luma2 - lumaC;
    bool is1Steepest = abs(gradient1) >= abs(gradient2);
    float gradientScaled = 0.25f * max(abs(gradient1), abs(gradient2));

    float stepLength = isHorizontal ? invSize.y : invSize.x;
    float lumaLocalAverage;
    if (is1Steepest)
    {
        stepLength = -stepLength;
        lumaLocalAverage = 0.5f * (luma1 + lumaC);
    }
    else
    {
        lumaLocalAverage = 0.5f * (luma2 + lumaC);
    }

    // エッジ中央へ半ピクセル寄せる
    float2 currentUV = uv;
    if (isHorizontal)
    {
        currentUV.y += stepLength * 0.5f;
    }
    else
    {
        currentUV.x += stepLength * 0.5f;
    }

    // エッジに沿って両側へ端点を探索する
    float2 offset = isHorizontal ? float2(invSize.x, 0) : float2(0, invSize.y);
    float2 uv1 = currentUV - offset;
    float2 uv2 = currentUV + offset;

    float lumaEnd1 = SampleLuma(uv1) - lumaLocalAverage;
    float lumaEnd2 = SampleLuma(uv2) - lumaLocalAverage;
    bool reached1 = abs(lumaEnd1) >= gradientScaled;
    bool reached2 = abs(lumaEnd2) >= gradientScaled;

    [loop]
    for (int i = 0; i < SEARCH_STEPS && !(reached1 && reached2); ++i)
    {
        if (!reached1)
        {
            uv1 -= offset;
            lumaEnd1 = SampleLuma(uv1) - lumaLocalAverage;
            reached1 = abs(lumaEnd1) >= gradientScaled;
        }
        if (!reached2)
        {
            uv2 += offset;
            lumaEnd2 = SampleLuma(uv2) - lumaLocalAverage;
            reached2 = abs(lumaEnd2) >= gradientScaled;
        }
    }

    // 端点までの距離からエッジ上の位置を求める
    float distance1 = isHorizontal ? (uv.x - uv1.x) : (uv.y - uv1.y);
    float distance2 = isHorizontal ? (uv2.x - uv.x) : (uv2.y - uv.y);
    bool isDirection1 = distance1 < distance2;
    float distanceFinal = min(distance1, distance2);
    float edgeLength = distance1 + distance2;
    float pixelOffset = -distanceFinal / edgeLength + 0.5f;

    // 端点のluma変動が中心と逆向きならオフセットを採用する
    bool isLumaCenterSmaller = lumaC < lumaLocalAverage;
    bool correctVariation = ((isDirection1 ? lumaEnd1 : lumaEnd2) < 0.0f) != isLumaCenterSmaller;
    float finalOffset = correctVariation ? pixelOffset : 0.0f;

    // サブピクセルAA（周囲平均との差分ベース）
    float lumaAverage = (1.0f / 12.0f) * (2.0f * (lumaD + lumaU + lumaL + lumaR) + lumaDL + lumaDR + lumaUL + lumaUR);
    float subPixelOffset1 = saturate(abs(lumaAverage - lumaC) / lumaRange);
    float subPixelOffset2 = (-2.0f * subPixelOffset1 + 3.0f) * subPixelOffset1 * subPixelOffset1;
    float subPixelOffsetFinal = subPixelOffset2 * subPixelOffset2 * SUBPIXEL_QUALITY;
    finalOffset = max(finalOffset, subPixelOffsetFinal);

    // 最終UVをずらしてサンプリング
    float2 finalUV = uv;
    if (isHorizontal)
    {
        finalUV.y += finalOffset * stepLength;
    }
    else
    {
        finalUV.x += finalOffset * stepLength;
    }

    float3 finalColor = g_SourceTex.SampleLevel(g_LinearClamp, finalUV, 0).rgb;
    return float4(finalColor, 1.0f);
}
