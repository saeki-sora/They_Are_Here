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

struct VS_IN
{
    float3 pos : POSITION0;
    float3 nrm : NORMAL0;
    float4 col : COLOR0;
    float2 tex : TEXCOORD0;
    uint4 boneIndices : BLENDINDICES0;
    float4 boneWeights : BLENDWEIGHT0;
};

struct PS_IN
{
    float4 pos : SV_POSITION;
    float4 col : COLOR0;
    float2 tex : TEXCOORD0;
};

struct LIGHT
{
    bool Enable;
    bool3 Dummy;
    float4 Direction;
    float4 Diffuse;
    float4 Ambient;
};

cbuffer LightBuffer : register(b11)
{
    LIGHT Light;
};

struct MATERIAL
{
    float4 Diffuse;
    float4 Ambient;
    float4 Specular;
    float4 Emission;
    float Shininess;
    bool TextureEnable;
    bool2 Dummy;
};

cbuffer MaterialBuffer : register(b4)
{
    MATERIAL Material;
};

//ボーン行列
static const uint MAX_BONES = 64;
cbuffer SkinningBuffer : register(b6)
{
    matrix BoneMatrices[MAX_BONES];
}


PS_IN vs_main(VS_IN input)
{
    PS_IN output;

    // -----------------------
    // スキニング
    // -----------------------
    float4 localPos = float4(input.pos, 1.0f);
    float4 localNrm = float4(input.nrm, 0.0f);

    float4 skinnedPos = 0;
    float3 skinnedNrm = 0;

    [unroll]
    for (int i = 0; i < 4; ++i)
    {
        float w = input.boneWeights[i];
        if (w <= 0.0f)
            continue;

        uint idx = input.boneIndices[i];
        matrix bm = BoneMatrices[idx];

        skinnedPos += mul(localPos, bm) * w;
        skinnedNrm += mul(localNrm, bm).xyz * w;
    }

    skinnedNrm = normalize(skinnedNrm);

    // -----------------------
    // WVP 変換
    // -----------------------
    matrix wvp = mul(mul(World, View), Projection);
    output.pos = mul(skinnedPos, wvp);

    // -----------------------
    // ライティング
    // -----------------------
    float4 worldNormal = mul(float4(skinnedNrm, 0.0f), World);
    worldNormal = normalize(worldNormal);

    float d = -dot(Light.Direction.xyz, worldNormal.xyz);
    d = saturate(d);

    output.col.xyz = input.col.xyz * d * Light.Diffuse.xyz;
    output.col.xyz += input.col.xyz * Light.Ambient.xyz;
    output.col.xyz += Material.Emission.xyz;
    output.col.a = input.col.a * Material.Diffuse.a;

    output.tex = input.tex;

    return output;
}
