struct PS_INPUT
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD0;
};

cbuffer ConstantBuffer : register(b1)
{
    float time;     // 累積時間
    float alpha;    // フェード値
    float2 padding;
};

// 疑似ノイズ
float hash(float2 p)
{
    return frac(sin(dot(p, float2(12.9898, 78.233))) * 43758.5453);
}

float4 ps_main(PS_INPUT input) : SV_TARGET
{
    float2 uv = input.uv;

    // -1〜1 に正規化した画面座標
    float2 p = uv * 2.0 - 1.0;
    float r = length(p); // 中心からの距離
    float angle = atan2(p.y, p.x); // 角度

    // ===== 放射状スピードライン =====

    // 角度方向のライン
    float lineCount = 40.0; // ラインの本数
    float flowSpeed = 3.0;  // 外へ流れる速度

    // メインライン
    float linePattern = frac(angle * lineCount / 6.28318 + time * 0.5);
    float lines = smoothstep(0.35, 0.45, linePattern) * smoothstep(0.65, 0.55, linePattern);

    // サブライン
    float subPattern = frac(angle * (lineCount * 1.7) / 6.28318 - time * 0.3);
    float subLines = smoothstep(0.40, 0.47, subPattern) * smoothstep(0.60, 0.53, subPattern);

    // 合成
    float combinedLines = saturate(lines * 0.7 + subLines * 0.3);

    // ===== 中心から外への流れ =====
    float flow = frac(r * 2.0 - time * flowSpeed);
    float flowMask = smoothstep(0.0, 0.3, flow) * smoothstep(1.0, 0.7, flow);
    combinedLines *= lerp(0.6, 1.0, flowMask);

    // ===== 画面端マスク（中央は透明、端ほど濃い）=====
    float edgeMask = smoothstep(0.25, 0.85, r);

    // 端の角を特に強調
    float cornerBoost = smoothstep(0.7, 1.2, r);
    edgeMask = lerp(edgeMask, 1.0, cornerBoost * 0.5);

    // ===== 微妙なゆらぎ =====
    float flicker = 0.9 + 0.1 * sin(time * 8.0 + angle * 3.0);

    // ===== 最終合成 =====
    float finalAlpha = combinedLines * edgeMask * flicker * alpha * 0.35;
    finalAlpha = saturate(finalAlpha);

    // 白めのスピードライン
    float3 lineColor = float3(0.9, 0.95, 1.0);

    return float4(lineColor, finalAlpha);
}
