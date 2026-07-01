cbuffer WorldBuffer : register(b0)
{
    matrix World;
}
cbuffer ViewBuffer : register(b1)
{
    matrix View;
}
cbuffer ProjBuffer : register(b2)
{
    matrix Projection;
}

struct VS_IN
{
    float3 pos : POSITION; // 頂点座標
    float3 nrm : NORMAL; // 使わないけどフォーマット合わせで必要
    float4 col : COLOR; // 頂点カラー
    float2 tex : TEXCOORD; // テクスチャ座標(UV)
};

struct PS_IN
{
    float4 pos : SV_POSITION;
    float4 col : COLOR;
    float2 tex : TEXCOORD;
};

PS_IN vs_main(VS_IN input)
{
    PS_IN output;
    
    // 行列計算
    matrix wvp = mul(World, View);
    wvp = mul(wvp, Projection);
    
    output.pos = mul(float4(input.pos, 1.0f), wvp);
    output.col = input.col;
    output.tex = input.tex;
    
    return output;
}