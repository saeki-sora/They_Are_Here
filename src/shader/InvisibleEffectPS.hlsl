struct PS_INPUT
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD0;
};

cbuffer ConstantBuffer : register(b1)
{
    float time; // 経過時間
    float totalTime; // 最大効果時間
    float2 padding;
};

// =========================
// 疑似ノイズ（靄のゆらぎ用）
// =========================
float hash(float2 p)
{
    return frac(sin(dot(p, float2(12.9898, 78.233))) * 43758.5453);
}

float noise2D(float2 p)
{
    float2 i = floor(p);
    float2 f = frac(p);

    float a = hash(i);
    float b = hash(i + float2(1.0, 0.0));
    float c = hash(i + float2(0.0, 1.0));
    float d = hash(i + float2(1.0, 1.0));

    float2 u = f * f * (3.0 - 2.0 * f);

    float ab = lerp(a, b, u.x);
    float cd = lerp(c, d, u.x);

    return lerp(ab, cd, u.y);
}

//フラクタルノイズ
float fbm(float2 p)
{
    float v = 0.0;
    float a = 0.5;

    [unroll]
    for (int i = 0; i < 4; ++i)
    {
        v += noise2D(p) * a;
        p = p * 2.0 + float2(13.5, 7.3);
        a *= 0.5;
    }
    return v;
}

float4 ps_main(PS_INPUT input) : SV_TARGET
{
    float2 uv = input.uv;

    // -1〜1 に正規化した画面座標
    float2 p = uv * 2.0 - 1.0;
    float r = length(p); // 中心からの距離

    // 時間正規化（0.0〜1.0）
    float tNorm = 0.0;
    if (totalTime > 0.001)
    {
        tNorm = saturate(time / totalTime);
    }

    // 出始めをぶわっと強く
    float appearBase = smoothstep(0.0, 0.25, tNorm);
    float appear = pow(appearBase, 0.6);

    // 終わり際は少しだけ薄くなる程度
    float fadeOut = 1.0 - smoothstep(0.85, 1.0, tNorm);

    //開始直後だけ少し濃くなるブースト
    float earlyBoost = lerp(1.8, 1.0, smoothstep(0.0, 0.35, tNorm));

    float timeFade = appear * fadeOut;

    // 靄の動き用の座標
    float2 flowUV = uv * 4.5; 
    flowUV += float2(time * 0.25, -time * 0.18);

    // 影の濃淡
    float shadowNoise = fbm(flowUV);
    float shadowNoise2 = fbm(flowUV * 1.7 + float2(-time * 0.15, time * 0.22));
    float shadowField = saturate(shadowNoise * 0.6 + shadowNoise2 * 0.6); // ★ ちょい増量

    // 画面の端ほど濃くなる
    float edgeMask = smoothstep(0.28, 0.95, r);

    // 真ん中もある程度暗く
    float centerHole = smoothstep(0.0, 0.30, r);
    float centerHoleFactor = lerp(0.7, 1.0, centerHole); //最小値を 0.4 → 0.7 に

    // 全体の影の強さ
    float shadowIntensity = shadowField * edgeMask * centerHoleFactor;

    // 少しだけゆらゆらさせるための追加の揺らぎ
    float pulsate = 0.9 + 0.1 * sin(time * 2.5);
    shadowIntensity *= pulsate;

    //アルファを全体的に強めに
    float alpha = shadowIntensity * 1.8 * timeFade * earlyBoost;
    alpha = saturate(alpha);

    // 色はほぼ黒。ほんの少しだけ青紫寄りの闇
    float3 shadowColor = float3(0.02, 0.02, 0.05);
    float3 mistColor = shadowColor;

    return float4(mistColor, alpha);
}
