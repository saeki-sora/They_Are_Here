// SSAO ピクセルシェーダー（深度バッファのみから遮蔽を計算する）
#include "postCommon.hlsl"

Texture2D g_DepthTex : register(t1);
SamplerState g_PointClamp : register(s1);

// 遮蔽判定のサンプル数
static const int SAMPLE_COUNT = 12;

// 深度からビュー空間位置を取得する
float3 FetchViewPos(float2 uv)
{
    float depth = g_DepthTex.SampleLevel(g_PointClamp, uv, 0).r;
    return ReconstructViewPos(uv, depth);
}

// 隣接ピクセルの差分からビュー空間法線を再構成する（差の小さい側を採用してエッジを回避）
float3 ReconstructNormal(float2 uv, float3 centerPos)
{
    float2 px = float2(PP_ScreenInfo.z, 0.0f) * 2.0f;
    float2 py = float2(0.0f, PP_ScreenInfo.w) * 2.0f;

    float3 right = FetchViewPos(uv + px);
    float3 left  = FetchViewPos(uv - px);
    float3 down  = FetchViewPos(uv + py);
    float3 up    = FetchViewPos(uv - py);

    // 中心との距離が近い側の差分を使う
    float3 ddxPos = (abs(right.z - centerPos.z) < abs(centerPos.z - left.z))
                        ? (right - centerPos) : (centerPos - left);
    float3 ddyPos = (abs(down.z - centerPos.z) < abs(centerPos.z - up.z))
                        ? (down - centerPos) : (centerPos - up);

    return normalize(cross(ddyPos, ddxPos));
}

float4 ps_main(in PS_IN_POST input) : SV_Target
{
    float depth = g_DepthTex.SampleLevel(g_PointClamp, input.tex, 0).r;

    // 遠景（スカイドーム等）は遮蔽なし
    if (depth >= 0.9999f)
    {
        return float4(1, 1, 1, 1);
    }

    float3 centerPos = ReconstructViewPos(input.tex, depth);
    float3 normal = ReconstructNormal(input.tex, centerPos);

    float radius = PP_SSAOParams.x;
    float bias = PP_SSAOParams.z;

    // ピクセル毎にサンプルパターンを回転させてバンディングを消す
    float rotation = InterleavedGradientNoise(input.pos.xy) * 6.2831853f;
    float sinR, cosR;
    sincos(rotation, sinR, cosR);

    // スクリーン空間でのサンプル半径（距離に応じて縮める）
    float screenRadius = radius * PP_Projection._11 / centerPos.z * 0.5f;

    float occlusion = 0.0f;

    [unroll]
    for (int i = 0; i < SAMPLE_COUNT; ++i)
    {
        // スパイラル状にサンプル点を配置する
        float ratio = (i + 0.5f) / SAMPLE_COUNT;
        float angle = ratio * 6.2831853f * 3.0f;
        float2 dir = float2(cos(angle) * cosR - sin(angle) * sinR,
                            cos(angle) * sinR + sin(angle) * cosR);
        float2 sampleUV = input.tex + dir * (ratio * screenRadius);

        float3 samplePos = FetchViewPos(sampleUV);
        float3 diff = samplePos - centerPos;
        float dist = length(diff);

        // 法線方向に手前へ張り出しているものを遮蔽とみなす
        float occ = max(0.0f, dot(normal, diff / max(dist, 0.0001f)) - bias);
        // 半径外のサンプルは寄与を減衰させる
        float falloff = saturate(1.0f - dist / radius);
        occlusion += occ * falloff;
    }

    occlusion = saturate(1.0f - occlusion / SAMPLE_COUNT * PP_SSAOParams.y);
    return float4(occlusion.xxx, 1.0f);
}
