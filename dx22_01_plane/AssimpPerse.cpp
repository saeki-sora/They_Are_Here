#include	<vector>
#include	<iostream>
#include	<unordered_map>
#include	<cassert>
#include	"Texture.h"
#include	"AssimpPerse.h"
#include "TextureManager.h"
#include<algorithm>
#include <cfloat>

#pragma comment(lib, "assimp-vc143-mtd.lib")

namespace AssimpPerse
{
	std::vector<std::vector<VERTEX>> g_vertices{};		// 頂点データ（メッシュ単位）
	std::vector<std::vector<unsigned int>> g_indices{};	// インデックスデータ（メッシュ単位）
	std::vector<SUBSET> g_subsets{};					// サブセット情報
	std::vector<MATERIAL> g_materials{};				// マテリアル
	std::vector<std::shared_ptr<Texture>> g_textures;	// ディフューズテクスチャ群

	DirectX::SimpleMath::Vector3 g_sceneMin(FLT_MAX, FLT_MAX, FLT_MAX);
	DirectX::SimpleMath::Vector3 g_sceneMax(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	// ディフューズTxtureコンテナを返す
	std::vector<std::shared_ptr<Texture>> GetTextures()
	{
		return std::move(g_textures);
	}

	// マテリアル情報をassimpを使用して取得する
	void GetMaterialData(const aiScene* pScene, const std::string& texturedirectory)
	{
		// 事前に正確なサイズを確保
		g_textures.clear();
		g_textures.reserve(pScene->mNumMaterials);
		g_materials.clear();
		g_materials.reserve(pScene->mNumMaterials);

		// マテリアル数文ループ
		for (unsigned int m = 0; m < pScene->mNumMaterials; m++)
		{
			const aiMaterial* material = pScene->mMaterials[m];

			// マテリアル名取得
			std::string mtrlname = material->GetName().C_Str();
			std::cout << mtrlname << std::endl;

			// マテリアル情報（デフォルト値で初期化）
			aiColor4D ambient(0.0f, 0.0f, 0.0f, 0.0f);
			aiColor4D diffuse(1.0f, 1.0f, 1.0f, 1.0f);
			aiColor4D specular(0.0f, 0.0f, 0.0f, 0.0f);
			aiColor4D emission(0.0f, 0.0f, 0.0f, 0.0f);
			float shiness = 0.0f;

			// マテリアルプロパティを一度に取得
			aiGetMaterialColor(material, AI_MATKEY_COLOR_AMBIENT, &ambient);
			aiGetMaterialColor(material, AI_MATKEY_COLOR_DIFFUSE, &diffuse);
			aiGetMaterialColor(material, AI_MATKEY_COLOR_SPECULAR, &specular);
			aiGetMaterialColor(material, AI_MATKEY_COLOR_EMISSIVE, &emission);
			aiGetMaterialFloat(material, AI_MATKEY_SHININESS, &shiness);

			// テクスチャ処理
			std::string texturename;
			// ローカル変数の型を shared_ptr に変更
			std::shared_ptr<Texture> texture = nullptr;

			unsigned int textureCount = material->GetTextureCount(aiTextureType_DIFFUSE);
			if (textureCount > 0)
			{
				aiString path{};
				if (AI_SUCCESS == material->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0), path))
				{
					std::string texpath(path.C_Str(), path.length);

					 // ---- ここから “安全化” ----
						 // 1) バックスラッシュ→スラッシュ
						for (auto& c : texpath) if (c == '\\') c = '/';
					
						 // 2) 制御文字・タブ・改行などを削除
						texpath.erase(std::remove_if(texpath.begin(), texpath.end(),
							[](unsigned char ch) { return ch < 0x20 || ch == 0x7F; }), texpath.end());
					
						 // 3) ディレクトリ付きなら **ファイル名だけ** 抜き出す
						 //    例: "Block/.003  sirokabe.jpg" → "sirokabe.jpg"
						std::filesystem::path p(texpath);
					std::string cleanName = p.filename().string();
					
						 // 4) 念のため前後の空白をトリム
						auto ltrim = [](std::string& s) { s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char c) {return c > 0x20; })); };
					auto rtrim = [](std::string& s) { s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char c) {return c > 0x20; }).base(), s.end()); };
					ltrim(cleanName); rtrim(cleanName);
					
						texturename = cleanName;   // ★ 保存するのは “安全化後の素のファイル名”
					 // ---- 安全化ここまで ----

					// テクスチャ読み込み
					if (auto tex = pScene->GetEmbeddedTexture(path.C_Str()))
					{
						// 内蔵テクスチャ
						texture = std::make_shared<Texture>();
						if (texture->LoadFromFemory((unsigned char*)tex->pcData, tex->mWidth))
						{
							std::cout << "Embedded" << std::endl;
						}
						else
						{
							texture.reset(); // shared_ptr のリセット
						}
					}
					else
					{
						// 外部テクスチャファイルは TextureManager に任せる！
						std::string texname = texturedirectory + "/" + texpath;
						texture = TextureManager::GetInstance().Load(texname); // この1行でOK!
						if (texture)
						{
							std::cout << "External texture loaded via Manager" << std::endl;
						}
					}
				}
			}

			// テクスチャが読み込めていれば g_textures に追加
			// texture が nullptr でもそのまま追加される (テクスチャなしマテリアル)
			g_textures.push_back(texture);

			// マテリアル情報を直接構築してpush_back
			g_materials.emplace_back(MATERIAL{
				std::move(mtrlname),
				ambient,
				diffuse,
				specular,
				emission,
				shiness,
				std::move(texturename)
				});
		}
	}

	void GetModelData(const std::string& filename, const std::string& texturedirectory)
	{
		// データをクリア（reserve付きで高速化）
		g_vertices.clear();
		g_indices.clear();
		g_subsets.clear();
		g_materials.clear();
		g_textures.clear();

		// AABBの値をリセット
		g_sceneMin = DirectX::SimpleMath::Vector3(FLT_MAX, FLT_MAX, FLT_MAX);
		g_sceneMax = DirectX::SimpleMath::Vector3(-FLT_MAX, -FLT_MAX, -FLT_MAX);

		// シーン情報構築
		Assimp::Importer importer;

		// シーン情報を構築
		const aiScene* pScene = importer.ReadFile(
			filename.c_str(),
			aiProcess_ConvertToLeftHanded |	// 左手座標系に変換する
			aiProcess_Triangulate);			// 三角形化する

		if (pScene == nullptr)
		{
			std::cout << "load error" << filename.c_str() << importer.GetErrorString() << std::endl;
		}
		assert(pScene != nullptr);

		// マテリアル情報取得
		GetMaterialData(pScene, texturedirectory);

		// メモリ事前確保
		g_vertices.resize(pScene->mNumMeshes);
		g_indices.resize(pScene->mNumMeshes);
		g_subsets.reserve(pScene->mNumMeshes);

		// メッシュ処理
		for (unsigned int m = 0; m < pScene->mNumMeshes; m++)
		{
			const aiMesh* mesh = pScene->mMeshes[m];
			const std::string meshname = mesh->mName.C_Str();
			const std::string& mtrlname = g_materials[mesh->mMaterialIndex].mtrlname;

			// 頂点データとインデックスデータのメモリを事前確保
			g_vertices[m].reserve(mesh->mNumVertices);
			g_indices[m].reserve(mesh->mNumFaces * 3); // 三角形なので3倍

			// 頂点処理
			for (unsigned int vidx = 0; vidx < mesh->mNumVertices; vidx++)
			{
				VERTEX v{};
				v.meshname = meshname;
				v.materialindex = mesh->mMaterialIndex;
				v.mtrlname = mtrlname;

				// 座標
				v.pos = mesh->mVertices[vidx];

				// AABB計算
				DirectX::SimpleMath::Vector3 currentPos(v.pos.x, v.pos.y, v.pos.z);
				g_sceneMin = DirectX::SimpleMath::Vector3::Min(g_sceneMin, currentPos);
				g_sceneMax = DirectX::SimpleMath::Vector3::Max(g_sceneMax, currentPos);

				// 法線
				v.normal = mesh->HasNormals() ? mesh->mNormals[vidx] : aiVector3D(0.0f, 0.0f, 0.0f);

				// 頂点カラー
				v.color = mesh->HasVertexColors(0) ? mesh->mColors[0][vidx] : aiColor4D(1.0f, 1.0f, 1.0f, 1.0f);

				// テクスチャ座標
				v.texcoord = mesh->HasTextureCoords(0) ? mesh->mTextureCoords[0][vidx] : aiVector3D(0.0f, 0.0f, 0.0f);

				g_vertices[m].push_back(v);
			}

			// インデックス処理
			for (unsigned int fidx = 0; fidx < mesh->mNumFaces; fidx++)
			{
				const aiFace& face = mesh->mFaces[fidx];
				assert(face.mNumIndices == 3);	// 三角形のみ対応

				// 3つのインデックスを一度に追加
				g_indices[m].insert(g_indices[m].end(),
					face.mIndices, face.mIndices + 3);
			}
		}

		// サブセット情報生成
		unsigned int vertexBase = 0;
		unsigned int indexBase = 0;

		for (unsigned int m = 0; m < pScene->mNumMeshes; m++)
		{
			SUBSET subset{};
			subset.IndexNum = static_cast<unsigned int>(g_indices[m].size());
			subset.VertexNum = static_cast<unsigned int>(g_vertices[m].size());
			subset.VertexBase = vertexBase;
			subset.IndexBase = indexBase;
			subset.meshname = g_vertices[m].empty() ? "" : g_vertices[m][0].meshname;
			subset.mtrlname = g_vertices[m].empty() ? "" : g_vertices[m][0].mtrlname;
			subset.materialindex = g_vertices[m].empty() ? 0 : g_vertices[m][0].materialindex;

			g_subsets.push_back(subset);

			// 次のメッシュのベース値を計算
			vertexBase += subset.VertexNum;
			indexBase += subset.IndexNum;
		}
	}

	// Getter関数群
	const std::vector<SUBSET>& GetSubsets()
	{
		return g_subsets;
	}

	const std::vector<std::vector<VERTEX>>& GetVertices()
	{
		return g_vertices;
	}

	const std::vector<std::vector<unsigned int>>& GetIndices()
	{
		return g_indices;
	}

	const std::vector<MATERIAL>& GetMaterials()
	{
		return g_materials;
	}

	DirectX::SimpleMath::Vector3 GetSceneMin()
	{
		return g_sceneMin;
	}

	DirectX::SimpleMath::Vector3 GetSceneMax()
	{
		return g_sceneMax;
	}
}