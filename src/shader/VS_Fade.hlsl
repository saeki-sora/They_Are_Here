struct VS_INPUT
{
    float4 Pos : POSITION;
    float2 Uv : TEXCOORD;
};

struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float2 Uv : TEXCOORD;
};

VS_OUTPUT vs_main(VS_INPUT input)
{
    VS_OUTPUT output = (VS_OUTPUT) 0;
    
    // í∏ì_ç¿ïWÇÇªÇÃÇ‹Ç‹èoóÕ
    output.Pos = input.Pos;
    output.Uv = input.Uv;
    return output;
}