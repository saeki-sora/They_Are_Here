#include "common.hlsl"

PS_IN vs_main(in VS_IN input)
{
    PS_IN output;
    
    matrix wvp;
    wvp = mul(World, View);
    wvp = mul(wvp, Projection);

    output.pos = mul(float4(input.pos, 1.0f), wvp);
    
    // UVÀ•W‚ğˆÚ“®‚³‚¹‚é
    float4 uv;
    uv.xy = input.tex;
    uv.z = 0.0f;
    uv.w = 1.0f;
    uv = mul(uv, matrixTex);
    output.tex = uv.xy;
    
    output.col = input.col;
    
    // PS_IN‚Ì\‘¢‘Ì‡‚í‚¹
    output.worldPos = float4(0.0f, 0.0f, 0.0f, 0.0f);
    output.normal = float3(0.0f, 0.0f, 0.0f);
    
    return output;
}