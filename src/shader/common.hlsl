// Common HLSL definitions for DirectX 11 shaders
#define MAX_POINT_LIGHTS 8

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
};

struct PS_IN
{
    float4 pos : SV_POSITION;
    float4 col : COLOR0;
    float2 tex : TEXCOORD0;
    float4 worldPos : TEXCOORD1;
    float3 normal : NORMAL0;
};

struct LIGHT
{
    bool Enable;
    float3 CameraPos;
    float4 Direction;
    float4 Diffuse;
    float4 Ambient;
};

struct DYNAMIC_LIGHT
{
    float4 Position;
    float4 Direction;
    float4 Color;
};

cbuffer LightBuffer : register(b3)
{
    LIGHT Light;
    DYNAMIC_LIGHT PointLights[MAX_POINT_LIGHTS];
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
}

cbuffer TextureMatrixBuffer : register(b5)
{
    matrix matrixTex;
}

cbuffer NormalMatrixBuffer : register(b7)
{
    matrix WorldInverseTranspose;
}