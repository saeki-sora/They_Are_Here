// 通常モデル用 ピクセルシェーダー
#include "common.hlsl"

Texture2D g_Texture : register(t0);
SamplerState g_SamplerState : register(s0);

// シャドウマップと比較サンプラー
Texture2D g_ShadowMap : register(t1);
SamplerComparisonState g_ShadowSampler : register(s1);

// ノーマルマップ
Texture2D g_NormalMap : register(t2);

// シャドウマップ解像度（C++側 Renderer::SHADOW_MAP_SIZE と合わせる）
static const float SHADOW_MAP_SIZE = 4096.0f;
// PCFのサンプル広がり（テクセル単位）
static const float PCF_FILTER_RADIUS = 2.0f;
// PCFのサンプル数
static const int PCF_SAMPLE_COUNT = 12;

// Poisson Disk サンプルパターン
static const float2 POISSON_DISK[12] =
{
    float2(-0.326f, -0.406f), float2(-0.840f, -0.074f),
    float2(-0.696f,  0.457f), float2(-0.203f,  0.621f),
    float2( 0.962f, -0.195f), float2( 0.473f, -0.480f),
    float2( 0.519f,  0.767f), float2( 0.185f, -0.893f),
    float2( 0.507f,  0.064f), float2( 0.896f,  0.412f),
    float2(-0.322f, -0.933f), float2(-0.792f, -0.598f)
};

// ピクセル位置ベースの疑似乱数（PCFのサンプル回転用）
float ShadowNoise(float2 pixelPos)
{
    const float3 magic = float3(0.06711056f, 0.00583715f, 52.9829189f);
    return frac(magic.z * frac(dot(pixelPos, magic.xy)));
}

// ノーマルマップで法線を変換する（Schüler法・タンジェント属性不要）
float3 PerturbNormal(float3 N, float3 viewVec, float2 uv)
{
    float3 mapN = g_NormalMap.Sample(g_SamplerState, uv).xyz * 2.0f - 1.0f;
    // 強度で凹凸の傾きを調整する
    mapN.xy *= RenderParams.x;

    // スクリーン空間微分からコタンジェントフレームを再構成する
    float3 dp1 = ddx(viewVec);
    float3 dp2 = ddy(viewVec);
    float2 duv1 = ddx(uv);
    float2 duv2 = ddy(uv);

    float3 dp2perp = cross(dp2, N);
    float3 dp1perp = cross(N, dp1);
    float3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    float3 B = dp2perp * duv1.y + dp1perp * duv2.y;

    float invmax = rsqrt(max(max(dot(T, T), dot(B, B)), 1e-10f));
    float3x3 TBN = float3x3(T * invmax, B * invmax, N);

    return normalize(mul(mapN, TBN));
}

float4 ps_main(in PS_IN input) : SV_Target
{
    // 法線の正規化
    float3 N = normalize(input.normal);

    // ノーマルマップ適用（テクスチャ付きマテリアルのみ）
    // テクスチャ無しマテリアルは t2 をバインドしないため、サンプルすると
    // 直前に描いたオブジェクトの古いノーマルマップが残りちらつく。頂点法線のままにする
    if (RenderParams.x > 0.0f && Material.TextureEnable)
    {
        N = PerturbNormal(N, input.worldPos.xyz - Light.CameraPos.xyz, input.tex);
    }

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

    // 鏡面光沢度。0 のままだと dot(N,H)<=0 の角度で pow(0,0)=NaN となり、
    // HDR+ブルームで黒い面が広範囲に広がるため下限を設ける
    float specPower = max(Material.Shininess, 1e-3f);

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
            float spec = pow(saturate(dot(N, H)), specPower);
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
                        float spec = pow(saturate(dot(N, H)), specPower);
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
            float texelSize = 1.0f / SHADOW_MAP_SIZE;

            // ピクセル毎にサンプルパターンを回転させてバンディングを消す
            float angle = ShadowNoise(input.pos.xy) * 6.2831853f;
            float sinR, cosR;
            sincos(angle, sinR, cosR);

            // Poisson Disk PCF サンプリング
            shadow = 0.0f;
            [unroll]
            for (int i = 0; i < PCF_SAMPLE_COUNT; ++i)
            {
                float2 p = POISSON_DISK[i];
                float2 rotated = float2(p.x * cosR - p.y * sinR, p.x * sinR + p.y * cosR);
                float2 offset = rotated * texelSize * PCF_FILTER_RADIUS;
                shadow += g_ShadowMap.SampleCmpLevelZero(g_ShadowSampler, shadowUV + offset, depth);
            }
            shadow /= PCF_SAMPLE_COUNT;
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
