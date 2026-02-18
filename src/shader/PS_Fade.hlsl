cbuffer CbFade : register(b0)
{
    float fadeAlpha;
    float3 padding;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float2 Uv : TEXCOORD;
};

// --- Noise Functions ---

float random(float2 st)
{
    return frac(sin(dot(st.xy, float2(12.9898, 78.233))) * 43758.5453123);
}

float noise(float2 st)
{
    float2 i = floor(st);
    float2 f = frac(st);

    // Smoothstep interpolation
    float2 u = f * f * (3.0 - 2.0 * f);

    return lerp(lerp(random(i + float2(0.0, 0.0)),
                     random(i + float2(1.0, 0.0)), u.x),
                lerp(random(i + float2(0.0, 1.0)),
                     random(i + float2(1.0, 1.0)), u.x), u.y);
}

// 軽量版 FBM (ループ回数を削減)
float fbm(float2 st)
{
    float value = 0.0;
    float amplitude = 0.5;
    
    // ERROR FIX: Reduced loop from 5 to 3 to fit in ps_2_x instruction limit
    for (int i = 0; i < 3; i++)
    {
        value += amplitude * noise(st);
        st *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}

// --- Main Shader ---

float4 ps_main(PS_INPUT input) : SV_Target
{
    float2 uv = input.Uv;

    // 1. Soot Movement
    float2 movement = float2(0.0, -fadeAlpha * 1.5);
    
    // 2. Generate Soot Pattern
    // Slightly simpler calculation
    float sootPattern = fbm((uv + movement) * 3.0);
    
    // (Removed fineSoot calculation to save instructions)

    // 3. Vignette
    float dist = distance(uv, float2(0.5, 0.5));
    float vignette = smoothstep(0.3, 1.0, dist);
    
    // 4. Combine
    float mask = sootPattern + (vignette * 0.5) + (1.0 - uv.y) * 0.3;

    // 5. Threshold
    float threshold = fadeAlpha * 1.8;

    // 6. Alpha Logic
    float alpha = 0.0;
    float edgeWidth = 0.2;
    
    if (mask < threshold - edgeWidth)
    {
        alpha = 1.0;
    }
    else if (mask < threshold)
    {
        float t = (threshold - mask) / edgeWidth;
        alpha = smoothstep(0.0, 1.0, t);
        alpha *= 0.9;
    }
    else
    {
        alpha = 0.0;
    }

    if (fadeAlpha >= 0.98)
        alpha = 1.0;

    return float4(0.0, 0.0, 0.0, alpha);
}