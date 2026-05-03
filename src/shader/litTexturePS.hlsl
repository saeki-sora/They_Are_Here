// 通常モデル用 ピクセルシェーダー
#include "common.hlsl"

Texture2D g_Texture : register(t0);
SamplerState g_SamplerState : register(s0);

// シャドウマップと比較サンプラー
Texture2D g_ShadowMap : register(t1);
SamplerComparisonState g_ShadowSampler : register(s1);

float4 ps_main(in PS_IN input) : SV_Target
{
    // 法線の正規化
    float3 N = normalize(input.normal);

    // テクスチャカラー取得
    float4 textureColor = float4(1, 1, 1, 1);
    if (Material.TextureEnable)
    {
        textureColor = g_Texture.Sample(g_SamplerState, input.tex);
    }

    // アンビエントで初期化
    float3 totalDiffuse = Light.Ambient.rgb;
    float3 totalSpecular = float3(0, 0, 0);
    float3 spotDiffuse = float3(0, 0, 0);   // 動的ライト(スポットライト)拡散光：影の影響を受けない
    float3 spotSpecular = float3(0, 0, 0);  // 動的ライト(スポットライト)鏡面光：影の影響を受けない

    // 視線ベクトル
    float3 ViewDir = normalize(Light.CameraPos - input.worldPos.xyz);

    // 平行光源
    if (Light.Enable)
    {
        float3 L = normalize(-Light.Direction.xyz);
        float d = dot(N, L);
        d = saturate(d);
        totalDiffuse += d * Light.Diffuse.rgb;

        // 平行光源のスペキュラー計算
        if (d > 0.0f)
        {
            float3 H = normalize(L + ViewDir);
            float spec = pow(saturate(dot(N, H)), Material.Shininess);
            totalSpecular += spec * Material.Specular.rgb * Light.Diffuse.rgb;
        }
    }

    // 動的ライトループ
    for (int i = 0; i < MAX_POINT_LIGHTS; ++i)
    {
        // タイプ取得 (0:無効, 1:点, 2:スポット)
        float lightType = PointLights[i].Color.w;

        if (lightType > 0.0f)
        {
            // ベクトル: ライト位置 - ピクセル位置
            float3 lightVec = PointLights[i].Position.xyz - input.worldPos.xyz;
            float dist = length(lightVec);
            float range = PointLights[i].Position.w;

            if (dist < range)
            {
                // 距離減衰
                float ratio = saturate(dist / range);
                float atten = 1.0f - ratio;
                atten = atten * atten;

                float3 L = normalize(lightVec);

                // スポットライト計算 (タイプ2の場合のみ)
                float spotFactor = 1.0f;
                if (lightType == 2.0f)
                {
                    float3 spotDir = normalize(PointLights[i].Direction.xyz);
                    float surfaceCos = dot(-L, spotDir);
                    float limitCos = PointLights[i].Direction.w;
                    spotFactor = smoothstep(limitCos, limitCos + 0.1f, surfaceCos);
                }

                atten *= spotFactor;

                if (atten > 0.0f)
                {
                    float lightIntensity = 2.0f;

                    // 拡散光（スポットライトは影に左右されないよう別変数へ）
                    float d = saturate(dot(N, L));
                    spotDiffuse += d * atten * PointLights[i].Color.rgb * lightIntensity;

                    // スペキュラー
                    if (d > 0.0f)
                    {
                        float3 H = normalize(L + ViewDir);
                        float spec = pow(saturate(dot(N, H)), Material.Shininess);
                        spotSpecular += spec * atten * Material.Specular.rgb * PointLights[i].Color.rgb * lightIntensity;
                    }
                }
            }
        }
    }

    // シャドウマップ判定（PCF 3x3）
    float shadow = 1.0f;
    float4 sc = input.shadowCoord;

    // ライト視錐台の内側のみ判定
    if (sc.w > 0.0f)
    {
        // NDC変換（-1〜1 → 0〜1 UV空間）
        float3 projCoords = sc.xyz / sc.w;
        float2 shadowUV;
        shadowUV.x = projCoords.x * 0.5f + 0.5f;
        shadowUV.y = -projCoords.y * 0.5f + 0.5f;

        // UV範囲内のみサンプリング
        if (shadowUV.x >= 0.0f && shadowUV.x <= 1.0f &&
            shadowUV.y >= 0.0f && shadowUV.y <= 1.0f)
        {
            float depth = projCoords.z - 0.0003f;  // シャドウアクネ防止バイアス
            float texelSize = 1.0f / 2048.0f;

            // PCF 3x3 サンプリング
            shadow = 0.0f;
            [unroll]
            for (int dx = -1; dx <= 1; ++dx)
            {
                [unroll]
                for (int dy = -1; dy <= 1; ++dy)
                {
                    float2 offset = float2(dx, dy) * texelSize;
                    shadow += g_ShadowMap.SampleCmpLevelZero(g_ShadowSampler, shadowUV + offset, depth);
                }
            }
            shadow /= 9.0f;
        }
    }

    // スポットライトが当たっている面では shadow を緩和（spotlight priority）
    float spotLuminance = dot(spotDiffuse, float3(0.333f, 0.333f, 0.334f));
    float adjustedShadow = max(shadow, saturate(spotLuminance) * 0.7f);

    // 影は平行光源にのみ適用。スポットライトは影に関係なく常に加算
    float3 lightContrib = Light.Ambient.rgb + adjustedShadow * (totalDiffuse - Light.Ambient.rgb) + spotDiffuse;

    // 最終カラー計算
    float4 finalColor;
    float3 baseColor = textureColor.rgb * input.col.rgb;
    finalColor.rgb = baseColor * lightContrib + totalSpecular * shadow + spotSpecular + Material.Emission.rgb;
    finalColor.a = textureColor.a * input.col.a * Material.Diffuse.a;

    // --- Fog ---
    static const float FOG_START  = 300.0f;
    static const float FOG_END    = 1200.0f;
    static const float3 FOG_COLOR = float3(0.02f, 0.02f, 0.05f);

    float fogDist = length(input.worldPos.xyz - Light.CameraPos.xyz);
    float fogFactor = saturate((FOG_END - fogDist) / (FOG_END - FOG_START));
    finalColor.rgb = lerp(FOG_COLOR, finalColor.rgb, fogFactor);


    return finalColor;
}
