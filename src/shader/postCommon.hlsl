// ポストプロセス共通定義（include専用）

#define MAX_VOLUMETRIC_LIGHTS 8 // ボリュメトリックライトの最大数

// ポストプロセス用定数バッファ
cbuffer PostProcessBuffer : register(b9)
{
    matrix PP_InvViewProj;      // ビュープロジェクション逆行列（ワールドレイ復元用）
    matrix PP_InvProjection;    // プロジェクション逆行列（ビュー空間復元用）
    matrix PP_Projection;       // プロジェクション行列（SSAO再投影用）
    float4 PP_ScreenInfo;       // xy:出力先サイズ zw:1/出力先サイズ
    float4 PP_ToneParams;       // x:露出 y:トーンマップバイパス z:彩度 w:経過時間
    float4 PP_BloomParams;      // x:輝度閾値 y:ソフトニー z:ブルーム強度 w:未使用
    float4 PP_BloomWeights;     // xyz:Mip毎の合成重み w:未使用
    float4 PP_BlurDir;          // xy:ブラー方向(テクセル単位) zw:未使用
    float4 PP_VignetteParams;   // x:ビネット強度 y:ビネット形状べき z:グレイン強度 w:未使用
    float4 PP_ColorTint;        // rgb:ティント色 w:ティント強度
    float4 PP_SSAOParams;       // x:半径(ビュー空間) y:強度 z:バイアス w:適用フラグ
    float4 PP_VolumetricParams; // x:散乱強度 y:ステップ数 z:レイ最大距離 w:適用フラグ
    float4 PP_CameraParams;     // x:ニアクリップ y:ファークリップ zw:未使用
    float4 PP_CameraPos;        // xyz:カメラワールド位置 w:未使用
}

// ボリュメトリックライト単体のデータ
struct VOLUMETRIC_LIGHT
{
    float4 Position;  // xyz:位置 w:範囲
    float4 Direction; // xyz:向き w:スポット角コサイン
    float4 Color;     // rgb:色 w:タイプ(0:無効 1:点 2:スポット)
};

// ボリュメトリックライト用定数バッファ
cbuffer VolumetricLightBuffer : register(b10)
{
    VOLUMETRIC_LIGHT VolLights[MAX_VOLUMETRIC_LIGHTS];
}

// フルスクリーン三角形VSの出力
struct PS_IN_POST
{
    float4 pos : SV_POSITION;
    float2 tex : TEXCOORD0;
};

// ハードウェア深度値をビュー空間Z（線形距離）へ変換する
float LinearizeDepth(float depth)
{
    float n = PP_CameraParams.x;
    float f = PP_CameraParams.y;
    // 透視投影行列の M33/M43 から逆算（LH透視投影前提）
    return n * f / (f - depth * (f - n));
}

// UVと深度からビュー空間位置を復元する
float3 ReconstructViewPos(float2 uv, float depth)
{
    float2 ndc = float2(uv.x * 2.0f - 1.0f, 1.0f - uv.y * 2.0f);
    float4 viewPos = mul(float4(ndc, depth, 1.0f), PP_InvProjection);
    return viewPos.xyz / viewPos.w;
}

// UVと深度からワールド空間位置を復元する
float3 ReconstructWorldPos(float2 uv, float depth)
{
    float2 ndc = float2(uv.x * 2.0f - 1.0f, 1.0f - uv.y * 2.0f);
    float4 worldPos = mul(float4(ndc, depth, 1.0f), PP_InvViewProj);
    return worldPos.xyz / worldPos.w;
}

// ACESフィルミック近似トーンマップ（Narkowicz）
float3 ACESFilm(float3 x)
{
    const float a = 2.51f;
    const float b = 0.03f;
    const float c = 2.43f;
    const float d = 0.59f;
    const float e = 0.14f;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

// Interleaved Gradient Noise（ピクセル位置ベースの疑似乱数）
float InterleavedGradientNoise(float2 pixelPos)
{
    const float3 magic = float3(0.06711056f, 0.00583715f, 52.9829189f);
    return frac(magic.z * frac(dot(pixelPos, magic.xy)));
}

// 輝度を求める（Rec.709）
float Luminance(float3 color)
{
    return dot(color, float3(0.2126f, 0.7152f, 0.0722f));
}
