#pragma once
#include "Object.h"
#include "MeshRenderer.h"
#include "Texture.h"
#include "Material.h"
#include"VisualObject.h"

//-----------------------------------------------------------------------------
// Arrowクラス
//-----------------------------------------------------------------------------
class Arrow :public VisualObject
{
private:

	// 描画の為の情報（メッシュに関わる情報）
	MeshRenderer m_MeshRenderer; // 頂点バッファ・インデックスバッファ・インデックス数

	// 描画の為の情報（見た目に関わる部分）
	std::vector<std::unique_ptr<Material>> m_Materiales;
	std::vector<SUBSET> m_subsets;
	std::vector<std::unique_ptr<Texture>> m_Textures; // テクスチャ

	int m_State = 0; // 0:非表示・1:方向選択・2:パワー選択

public:

	Arrow(
		const DirectX::SimpleMath::Vector3& pos = { 0, 30, 0 },
		const DirectX::SimpleMath::Vector3& size = { 1, 1, 1 }); // コンストラクタ
	~Arrow(); // デストラクタ

	void Init();
	void Update();
	void Draw();
	void Uninit();
	
	// 状態の設定
	void SetState(int s);

	// 矢印のベクトルを取得
	DirectX::SimpleMath::Vector3 GetVector();
};

