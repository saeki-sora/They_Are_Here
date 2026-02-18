// ==========================================
// 共通 ピクセルシェーダー
// ==========================================
#include "common.hlsl"

Texture2D g_Texture : register(t0);
SamplerState g_SamplerState : register(s0);

float4 ps_main(in PS_IN input) : SV_Target
{
    // 準備
    // 法線の正規化
    float3 N = normalize(input.normal);
    
    // テクスチャカラー取得
    float4 textureColor = float4(1, 1, 1, 1);
    if (Material.TextureEnable)
    {
        textureColor = g_Texture.Sample(g_SamplerState, input.tex);
    }
    
    // 環境光で初期化
    float3 totalDiffuse = Light.Ambient.rgb;
    float3 totalSpecular = float3(0, 0, 0);

    // 視線ベクトル
    float3 ViewDir = normalize(Light.CameraPos - input.worldPos.xyz);

    // 平行光源
    if (Light.Enable)
    {
        float3 L = normalize(-Light.Direction.xyz);
        float d = dot(N, L);
        d = saturate(d);
        totalDiffuse += d * Light.Diffuse.rgb;
        
        //平行光源のスペキュラー計算
        if (d > 0.0f)
        {
            float3 H = normalize(L + ViewDir); // ハーフベクトル
            float spec = pow(saturate(dot(N, H)), Material.Shininess); // Shininessで鋭さを調整
            totalSpecular += spec * Material.Specular.rgb * Light.Diffuse.rgb;
        }
    }

    
    // 光源ループ
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
                // 1. 距離減衰 (共通)
                float ratio = saturate(dist / range);
                float atten = 1.0f - ratio;
                atten = atten * atten;

                float3 L = normalize(lightVec); // ピクセルからライトへの向き

                // 2. スポットライト計算 (タイプ2の場合のみ)
                float spotFactor = 1.0f;
                if (lightType == 2.0f)
                {
                    // ライトの向き
                    float3 spotDir = normalize(PointLights[i].Direction.xyz);
                    
                    // ライトの向きと、ライトへのベクトル(逆向き)の内積
                    // dot(-L, spotDir) で、ライト正面からのズレを計算
                    float surfaceCos = dot(-L, spotDir);
                    float limitCos = PointLights[i].Direction.w; // 設定した角度のCos
                    
                    

                    // 境界をぼかす処理 (ソフトスポット)
                    // limitCos付近で滑らかに減衰させる
                    spotFactor = smoothstep(limitCos, limitCos + 0.1f, surfaceCos);
                }

                // 最終的な減衰 = 距離減衰 * スポット係数
                atten *= spotFactor;

                // 範囲内なら拡散光・スペキュラー計算
                if (atten > 0.0f)
                {
                    float lightIntensity = 2.0f;

                    // 拡散光
                    float d = saturate(dot(N, L));
                    totalDiffuse += d * atten * PointLights[i].Color.rgb * lightIntensity;

                    // スペキュラー
                    if (d > 0.0f)
                    {
                        float3 H = normalize(L + ViewDir);
                        float spec = pow(saturate(dot(N, H)), Material.Shininess);
                        totalSpecular += spec * atten * Material.Specular.rgb * PointLights[i].Color.rgb * lightIntensity;
                    }
                }
            }
        }
    }
    

    ////点光源ループ
    //for (int i = 0; i < MAX_POINT_LIGHTS; ++i)
    //{
    //    // 有効なライトか
    //    if (PointLights[i].Color.w > 0.0f)
    //    {
    //        // ベクトル: ライト位置 - ピクセル位置
    //        float3 lightVec = PointLights[i].Position.xyz - input.worldPos.xyz;
    //        float dist = length(lightVec);
    //        float range = PointLights[i].Position.w;

    //        if (dist < range)
    //        {
    //           // 減衰率の計算
    //            float ratio = saturate(dist / range);
    //            float atten = 1.0f - ratio;
    //            atten = atten * atten;

    //            // 光の強さ倍率
    //            float lightIntensity = 2.0f; // 点光源は強めに調整

    //            // 拡散光 (N dot L)
    //            float3 L = normalize(lightVec);
    //            float d = dot(N, L);
    //            d = saturate(d);

    //            // 加算 (Intensity を掛ける)
    //            totalDiffuse += d * atten * PointLights[i].Color.rgb * lightIntensity;
                
    //            if (d > 0.0f)
    //            {
    //                float3 H = normalize(L + ViewDir);
    //                float spec = pow(saturate(dot(N, H)), Material.Shininess);
    //                // 点光源も減衰を考慮してスペキュラーを加算
    //                totalSpecular += spec * atten * Material.Specular.rgb * PointLights[i].Color.rgb * lightIntensity;
    //            }
    //        }
    //    }
    //}


    //最終カラー合成
    float4 finalColor;
    
    // ベースカラー計算
    float3 baseColor = textureColor.rgb * input.col.rgb;

    finalColor.rgb = baseColor * totalDiffuse + Material.Emission.rgb;
    
    finalColor.rgb = baseColor * totalDiffuse + totalSpecular + Material.Emission.rgb;
    
    // アルファ
    finalColor.a = textureColor.a * input.col.a * Material.Diffuse.a;

    return finalColor;
}