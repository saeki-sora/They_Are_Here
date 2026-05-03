Texture2D g_MapTexture : register(t0);
Texture2D g_FogTexture : register(t1);
SamplerState g_Sampler : register(s0);

cbuffer UVAdjustBuffer : register(b5)
{
    float2 uvOffset;
    float2 uvScale;
    float  mapRotation;
    float3 _pad;
}

struct PS_IN
{
    float4 pos : SV_POSITION;
    float4 col : COLOR;
    float2 tex : TEXCOORD;
};

float4 ps_main(PS_IN input) : SV_TARGET
{
    // クワッド中心（プレイヤー位置）を軸にUVを回転してからズーム・オフセット適用
    float2 c = input.tex - float2(0.5f, 0.5f);
    float cosA = cos(mapRotation);
    float sinA = sin(mapRotation);
    float2 rotated;
    rotated.x = -(c.x * cosA - c.y * sinA);
    rotated.y = c.x * sinA + c.y * cosA;
    float2 rotatedTex = rotated + float2(0.5f, 0.5f);

    float2 adjustedUV = rotatedTex * uvScale + uvOffset;

    // UV範囲外はマップ外なので透明にする
    if (adjustedUV.x < 0.0f || adjustedUV.x > 1.0f || adjustedUV.y < 0.0f || adjustedUV.y > 1.0f)
    {
        discard;
    }

    float4 mapColor = g_MapTexture.Sample(g_Sampler, adjustedUV);

    float fogValue = g_FogTexture.Sample(g_Sampler, adjustedUV).r;

    mapColor.a *= fogValue;

    clip(mapColor.a - 0.01f);

    return mapColor;
}
