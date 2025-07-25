#include "common.hlsl"

PS_IN vs_main(in VS_IN input)
{
    PS_IN output;

	//positoin=============================
	// ワールド、ビュー、プロジェクション行列を掛け合わせて座標変換を行う
	matrix wvp;
	wvp = mul(World, View);
	wvp = mul(wvp, Projection);
    output.pos = mul(input.pos, wvp);
	
	//color=============================
	//法線ベクトルの変換と正規化を行う
	float4 normal = float4(input.nrm.xyz, 0.0f);
    float4 worldNormal = mul(normal, World);
	worldNormal = normalize(worldNormal);
	
	//拡散光を計算する
    float d = -dot(Light.Direction.xyz, worldNormal.xyz);
	
    d = saturate(d);
	
    output.col.xyz = input.col.xyz *d* Light.Diffuse.xyz; // 拡散光の影響を乗算
    output.col.xyz += input.col.xyz * Light.Ambient.xyz; // アンビエント光を加算
    output.col.xyz += Material.Emission.xyz;
    output.col.a = input.col.a*Material.Diffuse.a; // アルファ値はそのまま使用
	
	//texture=============================
	// テクスチャ座標はそのまま使用
    output.tex = input.tex;
	
    return output;
}

