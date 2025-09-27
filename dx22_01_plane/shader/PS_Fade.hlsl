// C++からフェードの透明度を受け取るための変数
cbuffer CbFade : register(b0)
{
    float fadeAlpha;
    float3 padding; // 16バイトアライメントのためのパディング
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float2 Uv : TEXCOORD;
};

float4 ps_main(PS_INPUT input) : SV_Target
{
    // R, G, B は 0.0 (黒) で、透明度を C++ から受け取った fadeAlpha にする
    return float4(0.0f, 0.0f, 0.0f, fadeAlpha);
}