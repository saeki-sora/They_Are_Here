// フルスクリーン三角形 頂点シェーダー（頂点バッファ不要、Draw(3,0)で使う）
#include "postCommon.hlsl"

PS_IN_POST vs_main(uint id : SV_VertexID)
{
    PS_IN_POST output;

    // 頂点ID 0,1,2 から画面全体を覆う巨大三角形を生成する
    float2 uv = float2((id << 1) & 2, id & 2);
    output.pos = float4(uv.x * 2.0f - 1.0f, 1.0f - uv.y * 2.0f, 0.0f, 1.0f);
    output.tex = uv;

    return output;
}
