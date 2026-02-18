cbuffer CbSoot : register(b8)
{
    matrix WorldViewProjection; // 行列
    float4 Color; // 色
    float Timer; // アニメーション進行度
    float3 Padding;
};

struct VS_INPUT
{
    float4 Pos : POSITION;
    float3 Normal : NORMAL; // VisualObjectの頂点データに合わせる
    float4 Color : COLOR;
    float2 Uv : TEXCOORD;
};

struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float2 Uv : TEXCOORD;
    float4 Color : COLOR;
};

VS_OUTPUT vs_main(VS_INPUT input)
{
    VS_OUTPUT output;
    output.Pos = mul(input.Pos, WorldViewProjection);
    output.Uv = input.Uv;
    output.Color = input.Color * Color; // 頂点カラーと設定カラーを乗算
    return output;
}