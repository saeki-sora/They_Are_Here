#include "common.hlsl"

PS_IN vs_main(in VS_IN input)
{
    PS_IN output;
	
    matrix wvp;
    wvp = mul(World, View);
    wvp = mul(wvp, Projection);

    output.pos = mul(input.pos, wvp);
    
    //UVç¿ïWÇà⁄ìÆÇ≥ÇπÇÈ

    float4 uv;
    uv.xy = input.tex;
    uv.z = 0.0f;
    uv.w = 1.0f;
    uv = mul(uv, matrixTex);
    output.tex = uv.xy;
    
    output.col = input.col;
	
    return output;
}

//#include "common.hlsl"

//PS_IN vs_main(in VS_IN input)
//{
//    PS_IN output;
	
//    matrix wvp;
//    wvp = mul(World, View);
//    wvp = mul(wvp, Projection);

//    output.pos = mul(input.pos, wvp);
//    output.tex = input.tex;
//    output.col = input.col;
	
//    return output;
//}


