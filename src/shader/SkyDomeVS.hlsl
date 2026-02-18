cbuffer WorldBuffer : register(b0)
{
    matrix World;
}
cbuffer ViewBuffer : register(b1)
{
    matrix View;
}
cbuffer ProjectionBuffer : register(b2)
{
    matrix Projection;
}

struct VS_INPUT
{
    float3 Pos : POSITION; // VERTEX_3D::position
    float3 Nrm : NORMAL; // VERTEX_3D::normal
    float4 Col : COLOR; // VERTEX_3D::color
    float2 Tex : TEXCOORD; // VERTEX_3D::uv
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEXCOORD;
};

PS_INPUT vs_main(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT) 0;

    matrix view = View;
    view[3][0] = 0;
    view[3][1] = 0;
    view[3][2] = 0;

    float4 pos = float4(input.Pos, 1.0f);
    pos = mul(pos, World);
    pos = mul(pos, view);
    pos = mul(pos, Projection);

    pos.z = pos.w; // ç≈âú
    output.Pos = pos;
    output.Tex = input.Tex;

    return output;
}
