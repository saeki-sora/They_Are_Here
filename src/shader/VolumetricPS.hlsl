// ボリュメトリックライト ピクセルシェーダー（スポット/点光源の光錐をレイマーチで可視化する）
#include "postCommon.hlsl"

Texture2D g_DepthTex : register(t1);
SamplerState g_PointClamp : register(s1);

// 1ステップ分の散乱寄与を計算する
float3 ScatterAtPoint(float3 worldPos)
{
    float3 total = float3(0, 0, 0);

    [unroll]
    for (int i = 0; i < MAX_VOLUMETRIC_LIGHTS; ++i)
    {
        float lightType = VolLights[i].Color.w;
        if (lightType <= 0.0f) continue;

        float3 lightVec = VolLights[i].Position.xyz - worldPos;
        float dist = length(lightVec);
        float range = VolLights[i].Position.w;
        if (dist >= range) continue;

        // 距離による二乗減衰
        float atten = 1.0f - saturate(dist / range);
        atten = atten * atten;

        // スポットライトは円錐の内側のみ
        if (lightType == 2.0f)
        {
            float3 L = lightVec / max(dist, 0.0001f);
            float surfaceCos = dot(-L, normalize(VolLights[i].Direction.xyz));
            float limitCos = VolLights[i].Direction.w;
            atten *= smoothstep(limitCos, limitCos + 0.05f, surfaceCos);
        }

        total += VolLights[i].Color.rgb * atten;
    }

    return total;
}

float4 ps_main(in PS_IN_POST input) : SV_Target
{
    float depth = g_DepthTex.SampleLevel(g_PointClamp, input.tex, 0).r;

    // レイの始点と終点（遮蔽物 or 最大距離まで）
    float3 rayStart = PP_CameraPos.xyz;
    float3 rayEnd = ReconstructWorldPos(input.tex, depth);
    float3 rayVec = rayEnd - rayStart;
    float rayLen = min(length(rayVec), PP_VolumetricParams.z);
    float3 rayDir = normalize(rayVec);

    int stepCount = (int) PP_VolumetricParams.y;
    float stepLen = rayLen / stepCount;

    // ディザでステップ開始位置をずらしバンディングを除去する
    float dither = InterleavedGradientNoise(input.pos.xy);

    float3 accum = float3(0, 0, 0);

    [loop]
    for (int i = 0; i < stepCount; ++i)
    {
        float t = (i + dither) * stepLen;
        float3 samplePos = rayStart + rayDir * t;
        accum += ScatterAtPoint(samplePos) * stepLen;
    }

    // 散乱強度で正規化（距離スケールを吸収するため1000で割る）
    accum *= PP_VolumetricParams.x / 1000.0f;

    return float4(accum, 1.0f);
}
