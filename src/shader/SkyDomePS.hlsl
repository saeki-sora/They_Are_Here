Texture2D shaderTexture : register(t0);
SamplerState samplerState : register(s0);

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEXCOORD;
};

float4 ps_main(PS_INPUT input) : SV_Target
{
    // テクスチャを使うなら
    float4 c = shaderTexture.Sample(samplerState, input.Tex);
    c.a = 1.0;
    return c;

}
