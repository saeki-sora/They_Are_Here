Texture2D g_Texture : register(t0);
SamplerState g_Sampler : register(s0);

cbuffer CbSoot : register(b8)
{
    matrix WorldViewProjection;
    float4 Color;
    float Timer; // 0.0 -> 1.0
    float3 Padding;
};

struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float2 Uv : TEXCOORD;
    float4 Color : COLOR;
};

// --- ノイズ関数 ---
float random(float2 st)
{
    return frac(sin(dot(st.xy, float2(12.9898, 78.233))) * 43758.5453123);
}
float noise(float2 st)
{
    float2 i = floor(st);
    float2 f = frac(st);
    float2 u = f * f * (3.0 - 2.0 * f);
    return lerp(lerp(random(i + float2(0.0, 0.0)), random(i + float2(1.0, 0.0)), u.x),
                lerp(random(i + float2(0.0, 1.0)), random(i + float2(1.0, 1.0)), u.x), u.y);
}
float fbm(float2 st)
{
    float value = 0.0;
    float amplitude = 0.5;
    for (int i = 0; i < 3; i++)
    {
        value += amplitude * noise(st);
        st *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}

float4 ps_main(VS_OUTPUT input) : SV_Target
{
    float4 texColor = g_Texture.Sample(g_Sampler, input.Uv);
    if (texColor.a <= 0.01)
        discard;

    float t = saturate(Timer); // 0..1

    // すす→消えていく方向（まずは現状のまま）
    float threshold = (t * 1.5) - 0.2;

    float2 flow = float2(0.0, -t * 0.2);
    float n = fbm((input.Uv + flow) * 8.0);

    // soot=1 が「すす強」、soot=0 が「綺麗」
    float soot = smoothstep(threshold - 0.2, threshold + 0.2, n);

    // reveal=0 は見えない、reveal=1 は完全に見える
    float reveal = 1.0 - soot;

    float3 baseRGB = texColor.rgb * input.Color.rgb;
    float3 sootRGB = float3(0.0, 0.0, 0.0);

    // 見た目も reveal で同期
    float3 rgb = lerp(sootRGB, baseRGB, reveal);

    // ★ここが重要：アルファも reveal で同期（＝文字が先に出ない）
    float a = texColor.a * input.Color.a * reveal;

    // 完了後は完全表示に固定
    if (t >= 1.0)
    {
        rgb = baseRGB;
        a = texColor.a * input.Color.a;
    }

    return float4(rgb, a);
}
