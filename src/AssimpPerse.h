#pragma once

#include "Renderer.h"
#include "SkinnedMesh.h"


namespace AssimpPerse
{
	struct VERTEX {
		std::string meshname;		// メッシュ名
		aiVector3D	pos;			// 位置
		aiVector3D	normal;			// 法線
		aiColor4D	color;			// 頂点カラー	
		aiVector3D	texcoord;		// テクスチャ座標	
		int			materialindex;	// マテリアルインデックス
		std::string mtrlname;		// マテリアル名
	};
	struct SUBSET {
		std::string meshname;		// メッシュ名
		int materialindex;			// マテリアルインデックス
		unsigned int VertexBase;	// 頂点バッファのベース
		unsigned int VertexNum;		// 頂点数
		unsigned int IndexBase;		// インデックスバッファのベース
		unsigned int IndexNum;		// インデックス数
		std::string	 mtrlname;		// マテリアル名
	};
	struct MATERIAL {
		std::string mtrlname;		// マテリアル名
		aiColor4D	Ambient;		// アンビエント
		aiColor4D	Diffuse;		// ディフューズ
		aiColor4D	Specular;		// スペキュラ
		aiColor4D	Emission;		// エミッション
		float		Shiness;		// シャイネス
		std::string texturename;	// テクスチャ名
	};


	//アニメ付きFBXを読みたい場合はこちらを使う
	void GetModelDataSkinned(const std::string& filename,
		const std::string& texturedirectory);


	//ゲッター
	void GetModelData(const std::string& filename, const std::string& texturedirectory);
	const std::vector<SUBSET>& GetSubsets();
	const std::vector<std::vector<VERTEX>>& GetVertices();
	const std::vector<std::vector<unsigned int>>& GetIndices();
	const std::vector<MATERIAL>& GetMaterials();
	std::vector<std::shared_ptr<Texture>> GetTextures();
	DirectX::SimpleMath::Vector3 GetSceneMin();
	DirectX::SimpleMath::Vector3 GetSceneMax();
	const std::vector<std::vector<VERTEX_SKINNED>>& GetSkinnedVertices();
	const Skeleton& GetSkeleton();
	const AnimationClip& GetAnimationClip(std::size_t index);
}