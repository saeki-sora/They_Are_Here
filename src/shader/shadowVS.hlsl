// スタティックメッシュ用シャドウマップ生成頂点シェーダー

cbuffer WorldBuffer : register(b0)
{
    matrix World;
}

cbuffer ShadowBuffer : register(b8)
{
    matrix LightViewProj;
}

float4 vs_main(float3 pos : POSITION0) : SV_POSITION
{
    // ワールド変換 → ライト視点クリップ座標
    float4 worldPos = mul(float4(pos, 1.0f), World);
    return mul(worldPos, LightViewProj);
}
