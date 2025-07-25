//cbuffer MatrixBuffer : register(b0)
//{
//    matrix World;
//    matrix View;
//    matrix Projection;
//}

//struct VS_INPUT
//{
//    float3 pos : POSITION;
//};

//struct PS_INPUT
//{
//    float4 pos : SV_POSITION;
//    float3 texCoord : TEXCOORD0;
//};

//PS_INPUT vs_main(VS_INPUT input)
//{
//    PS_INPUT output;

//    matrix wvp;
    
//    // View行列をローカル変数にコピー
//    matrix localView = View;
    
//    // ローカル変数の移動成分を除去
//    localView._41 = localView._42 = localView._43 = 0.0f;

//    wvp = mul(World, View);
//    wvp = mul(wvp, Projection);

//    output.pos = mul(float4(input.pos, 1.0f), wvp);
//    output.texCoord = input.pos;

//    return output;
//}
