// 通常モデル用 頂点シェーダー
#include "common.hlsl"

PS_IN vs_main(in VS_IN input)
{
    PS_IN output;

    // 座標変換
    matrix wvp;
    wvp = mul(World, View);
    wvp = mul(wvp, Projection);
    output.pos = mul(float4(input.pos, 1.0f), wvp);

    // ワールド座標
    output.worldPos = mul(float4(input.pos, 1.0f), World);

    float3 N = mul(input.nrm, (float3x3) WorldInverseTranspose);
    output.normal = normalize(N);

    // シャドウマップ座標
    output.shadowCoord = mul(output.worldPos, LightViewProj);

    // テクスチャ座標
    output.col = input.col;
    output.tex = input.tex;

    return output;
}
