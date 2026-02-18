Texture2D g_MapTexture : register(t0);
Texture2D g_FogTexture : register(t1);
SamplerState g_Sampler : register(s0);

// UV調整用の定数バッファ
cbuffer UVAdjustBuffer : register(b5)
{
    float2 uvOffset;
    float2 uvScale;
}

struct PS_IN
{
    float4 pos : SV_POSITION;
    float4 col : COLOR;
    float2 tex : TEXCOORD;
};

float4 ps_main(PS_IN input) : SV_TARGET
{
    // UV座標を調整してズームとオフセットを適用
    float2 adjustedUV = input.tex * uvScale + uvOffset;
    
    // UV座標が範囲外の場合は透明にする
    if (adjustedUV.x < 0.0f || adjustedUV.x > 1.0f || adjustedUV.y < 0.0f || adjustedUV.y > 1.0f)
    {
        discard;
    }
    
    // 地図の色を取得
    float4 mapColor = g_MapTexture.Sample(g_Sampler, adjustedUV);
    
    // 探索状況を取得
    float fogValue = g_FogTexture.Sample(g_Sampler, adjustedUV).r;
    
    // 未探索エリアは見えなくする
    mapColor.a *= fogValue;
    
    // 透明な場所は描画しない
    clip(mapColor.a - 0.01f);
    
    return mapColor;
}