struct VS_IN
{
    float3 pos : POSITION; // 頂点座標
    float3 nrm : NORMAL; // 法線
    float4 col : COLOR; // 頂点カラー
    float2 uv : TEXCOORD0; // テクスチャ座標
};

// ピクセルシェーダに送るデータ
struct PS_INPUT
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD0;
};

PS_INPUT vs_main(VS_IN input)
{
    PS_INPUT output;

    output.position = float4(input.pos.x * 2.0, input.pos.y * 2.0, 0.0, 1.0);

    // 色とUVはそのままパス
    output.color = input.col;
    output.uv = input.uv;

    return output;
}