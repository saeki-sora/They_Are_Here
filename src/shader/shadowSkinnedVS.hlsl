// スキニングメッシュ用シャドウマップ生成頂点シェーダー

cbuffer WorldBuffer : register(b0)
{
    matrix World;
}

cbuffer ShadowBuffer : register(b8)
{
    matrix LightViewProj;
}

// ボーン行列
static const uint MAX_BONES = 64;
cbuffer SkinningBuffer : register(b6)
{
    matrix BoneMatrices[MAX_BONES];
}

struct VS_IN
{
    float3 pos          : POSITION0;
    float3 nrm          : NORMAL0;
    float4 col          : COLOR0;
    float2 tex          : TEXCOORD0;
    uint4  boneIndices  : BLENDINDICES0;
    float4 boneWeights  : BLENDWEIGHT0;
};

float4 vs_main(VS_IN input) : SV_POSITION
{
    // スキニング計算
    float4 localPos = float4(input.pos, 1.0f);
    float4 skinnedPos = float4(0, 0, 0, 0);

    [unroll]
    for (int i = 0; i < 4; ++i)
    {
        float w = input.boneWeights[i];
        if (w <= 0.0f)
            continue;
        uint idx = input.boneIndices[i];
        skinnedPos += mul(localPos, BoneMatrices[idx]) * w;
    }

    // ワールド変換 → ライト視点クリップ座標
    float4 worldPos = mul(skinnedPos, World);
    return mul(worldPos, LightViewProj);
}
