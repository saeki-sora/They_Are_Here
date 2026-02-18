struct PS_INPUT
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD0;
};

cbuffer ConstantBuffer : register(b1)
{
    float time; // 経過時間
    float intensity; // エフェクトの強さ
    float2 padding;
}

float4 ps_main(PS_INPUT input) : SV_TARGET
{
    //基本設定
    float2 center = float2(0.5, 0.5);
    float dist = distance(input.uv, center);

    //警告色の作成
    // 中心は透明、外側に行くほど赤くする
    float vignette = smoothstep(0.2, 0.9, dist);
    
    // 四隅をより濃く
    vignette = pow(vignette, 1.5);

    
    // 時間経過で点滅させる
    float pulse = (sin(time * 20.0) + 1.0) * 0.5;
    pulse = 0.5 + 0.5 * pulse; 

    // ノイズ
    float scanline = sin(input.uv.y * 800.0 + time * 10.0) * 0.1;

    // 色の合成 
    float3 redColor = float3(1.0, 0.0, 0.1); // 赤
    
    // エフェクト強度を掛けて、時間経過でフェードアウトさせる
    float alpha = (vignette * pulse + scanline) * intensity;

    // アルファ値のリミット
    alpha = saturate(alpha * 0.8); // 最大濃度調整

    return float4(redColor, alpha);
}