#include "pch.h"
#include	"Texture.h"
#include	"AssimpPerse.h"
#include "TextureManager.h"


#pragma comment(lib, "assimp-vc143-mtd.lib")

// ==========================================================================
// グローバル変数の定義
// ==========================================================================
namespace AssimpPerse
{
	std::vector<std::vector<VERTEX>> g_vertices{};		// 頂点データ（メッシュ単位）
	std::vector<std::vector<unsigned int>> g_indices{};	// インデックスデータ（メッシュ単位）
	std::vector<SUBSET> g_subsets{};					// サブセット情報
	std::vector<MATERIAL> g_materials{};				// マテリアル
	std::vector<std::shared_ptr<Texture>> g_textures;	// ディフューズテクスチャ群

	static std::vector<std::vector<VERTEX_SKINNED>> g_verticesSkinned;// スキニング頂点データ（メッシュ単位）
	static Skeleton g_skeleton;// スケルトン情報
	static std::vector<AnimationClip> g_clips;// アニメーションクリップ群

	DirectX::SimpleMath::Vector3 g_sceneMin(FLT_MAX, FLT_MAX, FLT_MAX);
	DirectX::SimpleMath::Vector3 g_sceneMax(-FLT_MAX, -FLT_MAX, -FLT_MAX);
}

// ==========================================================================
// 内部関数の定義
// ==========================================================================
namespace
{
	using DirectX::SimpleMath::Matrix;
	using DirectX::SimpleMath::Vector3;
	using DirectX::SimpleMath::Quaternion;

	Matrix AiToMatrix(const aiMatrix4x4& m)
	{
		return Matrix(
			m.a1, m.b1, m.c1, m.d1,
			m.a2, m.b2, m.c2, m.d2,
			m.a3, m.b3, m.c3, m.d3,
			m.a4, m.b4, m.c4, m.d4
		);
	}

	// 頂点 1 つにボーンウェイトを追加（最大4本）
	void AddBoneWeight(VERTEX_SKINNED& v, int boneIndex, float weight)
	{
		if (weight <= 0.0f) return;

		// 空きスロットがあればそこに入れる
		for (int i = 0; i < 4; ++i)
		{
			if (v.boneWeight[i] == 0.0f)
			{
				v.boneIndex[i] = static_cast<uint8_t>(boneIndex);
				v.boneWeight[i] = weight;
				return;
			}
		}

		// すでに4本埋まっている場合は、最小ウェイトと比較し、
		// 新しい方が大きければ差し替える
		int   smallest = 0;
		float minW = v.boneWeight[0];

		for (int i = 1; i < 4; ++i)
		{
			if (v.boneWeight[i] < minW)
			{
				smallest = i;
				minW = v.boneWeight[i];
			}
		}

		if (weight > minW)
		{
			v.boneIndex[smallest] = static_cast<uint8_t>(boneIndex);
			v.boneWeight[smallest] = weight;
		}
	}

	// Skeleton を aiScene から構築
	void BuildSkeletonFromScene(
		const aiScene* scene,
		Skeleton& outSkeleton,
		std::unordered_map<std::string, int>& outBoneIndexMap)
	{
		outSkeleton.bones.clear();
		outBoneIndexMap.clear();

		// 一時的に offsetMatrix を覚えておく
		struct TempBone
		{
			aiMatrix4x4 offset;
		};
		std::unordered_map<std::string, TempBone> tempBones;

		//全メッシュを走査して「ボーン一覧」を作る
		for (unsigned int m = 0; m < scene->mNumMeshes; ++m)
		{
			const aiMesh* mesh = scene->mMeshes[m];

			for (unsigned int b = 0; b < mesh->mNumBones; ++b)
			{
				const aiBone* bone = mesh->mBones[b];
				std::string   name = bone->mName.C_Str();

				if (outBoneIndexMap.find(name) != outBoneIndexMap.end())
				{
					continue; // 既に登録済み
				}

				int idx = static_cast<int>(outSkeleton.bones.size());
				outBoneIndexMap[name] = idx;

				Bone dst{};
				dst.name = name;
				dst.parentIndex = -1; // 後で埋める

				outSkeleton.bones.push_back(dst);

				tempBones[name].offset = bone->mOffsetMatrix;
			}
		}

		//親子関係と BindPose を決定
		for (auto& kv : outBoneIndexMap)
		{
			const std::string& boneName = kv.first;
			int                index = kv.second;

			// 親インデックス
			const aiNode* node = scene->mRootNode->FindNode(boneName.c_str());
			if (node && node->mParent)
			{
				std::string parentName = node->mParent->mName.C_Str();
				auto itParent = outBoneIndexMap.find(parentName);
				if (itParent != outBoneIndexMap.end())
				{
					outSkeleton.bones[index].parentIndex = itParent->second;
				}
			}

			//OffsetMatrixは逆バインドポーズ
			auto itTmp = tempBones.find(boneName);
			if (itTmp != tempBones.end())
			{
				Matrix invBind = AiToMatrix(itTmp->second.offset);
				outSkeleton.bones[index].inverseBindPose = invBind;
				outSkeleton.bones[index].bindPose = invBind.Invert();
			}
		}
	}

	// スキニング用頂点・インデックス・サブセットを構築
	void BuildSkinnedMeshData(
		const aiScene* scene,
		const Skeleton& skeleton,
		const std::unordered_map<std::string, int>& boneIndexMap)
	{
		using namespace AssimpPerse;

		g_verticesSkinned.clear();
		g_indices.clear();
		g_subsets.clear();

		g_verticesSkinned.resize(scene->mNumMeshes);
		g_indices.resize(scene->mNumMeshes);

		// AABB 初期化
		g_sceneMin = Vector3(FLT_MAX, FLT_MAX, FLT_MAX);
		g_sceneMax = Vector3(-FLT_MAX, -FLT_MAX, -FLT_MAX);

		unsigned int vertexBase = 0;
		unsigned int indexBase = 0;

		for (unsigned int m = 0; m < scene->mNumMeshes; ++m)
		{
			const aiMesh* mesh = scene->mMeshes[m];
			const std::string meshName = mesh->mName.C_Str();

			auto& dstVertices = g_verticesSkinned[m];
			auto& dstIndices = g_indices[m];

			dstVertices.resize(mesh->mNumVertices);
			dstIndices.reserve(mesh->mNumFaces * 3);

			//頂点基本情報
			for (unsigned int vidx = 0; vidx < mesh->mNumVertices; ++vidx)
			{
				VERTEX_SKINNED v{};

				// 位置
				const aiVector3D& p = mesh->mVertices[vidx];
				v.position = Vector3(p.x, p.y, p.z);

				// 法線
				if (mesh->HasNormals())
				{
					const aiVector3D& n = mesh->mNormals[vidx];
					v.normal = Vector3(n.x, n.y, n.z);
				}
				else
				{
					v.normal = Vector3(0.0f, 1.0f, 0.0f);
				}

				// カラー
				if (mesh->HasVertexColors(0))
				{
					const aiColor4D& c = mesh->mColors[0][vidx];
					v.color = DirectX::SimpleMath::Color(c.r, c.g, c.b, c.a);
				}
				else
				{
					v.color = DirectX::SimpleMath::Color(1, 1, 1, 1);
				}

				// UV
				if (mesh->HasTextureCoords(0))
				{
					const aiVector3D& t = mesh->mTextureCoords[0][vidx];
					v.uv = DirectX::SimpleMath::Vector2(t.x, t.y);
				}
				else
				{
					v.uv = DirectX::SimpleMath::Vector2(0, 0);
				}

				// ボーン情報は一旦「全部 0」にしておく
				for (int i = 0; i < 4; ++i)
				{
					v.boneIndex[i] = 0;
					v.boneWeight[i] = 0.0f;
				}

				dstVertices[vidx] = v;

				// AABB 更新
				g_sceneMin = Vector3::Min(g_sceneMin, v.position);
				g_sceneMax = Vector3::Max(g_sceneMax, v.position);
			}

			// ------ ボーンウェイトの割り当て ------
			for (unsigned int b = 0; b < mesh->mNumBones; ++b)
			{
				const aiBone* bone = mesh->mBones[b];
				std::string boneName = bone->mName.C_Str();

				auto itIdx = boneIndexMap.find(boneName);
				if (itIdx == boneIndexMap.end())
				{
					continue; // スケルトンに登録されていないボーンは無視
				}
				int boneIndex = itIdx->second;

				for (unsigned int w = 0; w < bone->mNumWeights; ++w)
				{
					const aiVertexWeight& vw = bone->mWeights[w];
					unsigned int vId = vw.mVertexId;
					float        weight = vw.mWeight;

					if (vId >= dstVertices.size()) continue;

					AddBoneWeight(dstVertices[vId], boneIndex, weight);
				}
			}

			// ウェイトが全て 0 の頂点には「ボーン0を重み1」で割り当て
			for (auto& v : dstVertices)
			{
				float sum = v.boneWeight[0] + v.boneWeight[1] +
					v.boneWeight[2] + v.boneWeight[3];
				if (sum == 0.0f)
				{
					v.boneIndex[0] = 0;
					v.boneWeight[0] = 1.0f;
				}
			}

			// ------ インデックス ------
			for (unsigned int fidx = 0; fidx < mesh->mNumFaces; ++fidx)
			{
				const aiFace& face = mesh->mFaces[fidx];
				assert(face.mNumIndices == 3);

				dstIndices.insert(
					dstIndices.end(),
					face.mIndices,
					face.mIndices + 3);
			}

			// ------ サブセット情報 ------
			AssimpPerse::SUBSET subset{};
			subset.IndexNum = static_cast<unsigned int>(dstIndices.size());
			subset.VertexNum = static_cast<unsigned int>(dstVertices.size());
			subset.VertexBase = vertexBase;
			subset.IndexBase = indexBase;
			subset.meshname = meshName;
			subset.materialindex = mesh->mMaterialIndex;

			subset.mtrlname = g_materials[mesh->mMaterialIndex].mtrlname;

			g_subsets.push_back(subset);

			vertexBase += subset.VertexNum;
			indexBase += subset.IndexNum;
		}
	}

	// アニメーションクリップを構築
	void BuildAnimationClipsFromScene(
		const aiScene* scene,
		const Skeleton& skeleton,
		std::vector<AnimationClip>& outClips)
	{
		outClips.clear();

		if (scene->mNumAnimations == 0)
		{
			return;
		}

		for (unsigned int a = 0; a < scene->mNumAnimations; ++a)
		{
			const aiAnimation* anim = scene->mAnimations[a];

			AnimationClip clip{};
			clip.name = anim->mName.C_Str();
			if (clip.name.empty())
			{
				clip.name = "Anim" + std::to_string(a);
			}

			clip.duration = static_cast<float>(anim->mDuration);
			clip.ticksPerSecond = (anim->mTicksPerSecond > 0.0)
				? static_cast<float>(anim->mTicksPerSecond)
				: 30.0f;

			clip.boneAnimations.resize(skeleton.bones.size());

			// 各ボーンのチャンネルを詰める
			for (unsigned int c = 0; c < anim->mNumChannels; ++c)
			{
				const aiNodeAnim* channel = anim->mChannels[c];
				std::string boneName = channel->mNodeName.C_Str();

				int boneIndex = skeleton.FindBoneIndex(boneName);
				if (boneIndex < 0) continue;

				BoneAnimation& dst = clip.boneAnimations[boneIndex];

				// キーフレーム数は Position, Rotation, Scaling の最大値に合わせる
				const unsigned int numPos = channel->mNumPositionKeys;
				const unsigned int numRot = channel->mNumRotationKeys;
				const unsigned int numScl = channel->mNumScalingKeys;
				const unsigned int numKey = std::max({ numPos, numRot, numScl });

				dst.keyframes.reserve(numKey);

				for (unsigned int i = 0; i < numKey; ++i)
				{
					Keyframe kf{};

					// 時間は Position キーを優先、なければ他で代用
					if (i < numPos)
						kf.time = static_cast<float>(channel->mPositionKeys[i].mTime);
					else if (i < numRot)
						kf.time = static_cast<float>(channel->mRotationKeys[i].mTime);
					else if (i < numScl)
						kf.time = static_cast<float>(channel->mScalingKeys[i].mTime);
					else
						kf.time = 0.0f;

					// 平行移動
					if (i < numPos)
					{
						const aiVectorKey& pk = channel->mPositionKeys[i];
						kf.translation = Vector3(
							static_cast<float>(pk.mValue.x),
							static_cast<float>(pk.mValue.y),
							static_cast<float>(pk.mValue.z));
					}

					// 回転
					if (i < numRot)
					{
						const aiQuatKey& rk = channel->mRotationKeys[i];
						kf.rotation = Quaternion(
							static_cast<float>(rk.mValue.x),
							static_cast<float>(rk.mValue.y),
							static_cast<float>(rk.mValue.z),
							static_cast<float>(rk.mValue.w));
					}

					// スケール
					if (i < numScl)
					{
						const aiVectorKey& sk = channel->mScalingKeys[i];
						kf.scale = Vector3(
							static_cast<float>(sk.mValue.x),
							static_cast<float>(sk.mValue.y),
							static_cast<float>(sk.mValue.z));
					}

					dst.keyframes.push_back(kf);
				}
			}

			outClips.push_back(std::move(clip));
		}
	}
}

// ==========================================================================
// メインの処理関数 
// ==========================================================================
namespace AssimpPerse
{
	// ディフューズTxtureコンテナを返す
	std::vector<std::shared_ptr<Texture>> GetTextures()
	{
		return std::move(g_textures);
	}

	// マテリアル情報をassimpを使用して取得する
	void GetMaterialData(const aiScene* pScene, const std::string& texturedirectory, const std::string& modelFilename)
	{
		// 事前に正確なサイズを確保
		g_textures.clear();
		g_textures.reserve(pScene->mNumMaterials);
		g_materials.clear();
		g_materials.reserve(pScene->mNumMaterials);

		// モデルファイル名の拡張子なしの部分を取得
		std::filesystem::path modelPath(modelFilename);
		std::string modelStem = modelPath.stem().string();

		// マテリアル数文ループ
		for (unsigned int m = 0; m < pScene->mNumMaterials; m++)
		{
			const aiMaterial* material = pScene->mMaterials[m];

			// マテリアル名取得
			std::string mtrlname = material->GetName().C_Str();
#ifdef _DEBUG
			std::cout << mtrlname << std::endl;
#endif

			// マテリアル情報
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

			std::string texturenameToSave; //バイナリに保存するファイル名
			std::string texturePathToLoad; //TextureManagerが今ロードするフルパス
			std::shared_ptr<Texture> texture = nullptr;

			unsigned int textureCount = material->GetTextureCount(aiTextureType_DIFFUSE);
			if (textureCount > 0)
			{
				aiString path{};
				if (AI_SUCCESS == material->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0), path))
				{
					std::string texpath_raw(path.C_Str(), path.length);

					// 安全化処理
					for (auto& c : texpath_raw) if (c == '\\') c = '/';
					texpath_raw.erase(std::remove_if(texpath_raw.begin(), texpath_raw.end(),
						[](unsigned char ch) { return ch < 0x20 || ch == 0x7F; }), texpath_raw.end());
					std::filesystem::path p(texpath_raw);
					std::string cleanName = p.filename().string();
					auto ltrim = [](std::string& s) { s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char c) {return c > 0x20; })); };
					auto rtrim = [](std::string& s) { s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char c) {return c > 0x20; }).base(), s.end()); };
					ltrim(cleanName); rtrim(cleanName);


					if (auto tex = pScene->GetEmbeddedTexture(path.C_Str()))
					{
						// ----- 埋め込みテクスチャの場合 -----

						// 保存するファイル名と形式を決定
						// (例: ".png" や ".jpg")
						std::string formatHint = (tex->achFormatHint[0] == 0) ? "png" : tex->achFormatHint;
						// (例: "FloorBlock2_embed_0.png")
						std::string newFilename = modelStem + "_embed_" + std::to_string(m) + "." + formatHint;

						// 保存するフルパスを決定
						// (例: "assets/model/FloorBlock/FloorBlock2_embed_0.png")
						std::filesystem::path newFullPath = std::filesystem::path(texturedirectory) / newFilename;
						std::string newFullPathStr = newFullPath.lexically_normal().string();

						// ファイルがまだ存在しない場合のみ、データを書き込む
						if (!std::filesystem::exists(newFullPath))
						{
	#ifdef _DEBUG
						std::cout << "Extracting embedded texture: " << newFullPathStr << std::endl;
#endif
							std::ofstream ofs(newFullPath, std::ios::binary);
							if (ofs)
							{
								// tex->mWidth は「データ長」
								ofs.write(reinterpret_cast<const char*>(tex->pcData), tex->mWidth);
								ofs.close();
							}
							else
							{
								std::cerr << "Failed to write embedded texture: " << newFullPathStr << std::endl;
							}
						}

						// 保存用/ロード用のパスを設定
						texturenameToSave = newFilename;     //バイナリにはこの名前を保存
						texturePathToLoad = newFullPathStr;  //今回はこのパスからロード

					}
					else if (!texpath_raw.empty()) // cleanName ではなく texpath_raw でチェック
					{
						// ----- 外部ファイルテクスチャの場合 -----

						// 保存はファイル名だけ (元のロジックと同じ)
						texturenameToSave = cleanName;

						// ロードパスは texpath_raw (FBX内のパス) を使う (元のロジックに戻す)
						texturePathToLoad = (std::filesystem::path(texturedirectory) / texpath_raw).lexically_normal().string();
					}
				}
			}

			// テクスチャ読み込み
			if (!texturePathToLoad.empty())
			{
				texture = TextureManager::GetInstance().Load(texturePathToLoad);
				if (!texture)
				{
#ifdef _DEBUG
					std::cout << "TextureManager failed to load: " << texturePathToLoad << std::endl;
#endif
					// 失敗した場合でもデフォルトテクスチャをロード
					texture = TextureManager::GetInstance().Load("");
				}
			}
			else
			{
				// テクスチャパスが空 (テクスチャなしマテリアル)
				texture = TextureManager::GetInstance().Load(""); // デフォルトテクスチャをロード
			}

			g_textures.push_back(texture);// テクスチャが読み込めていれば g_textures に追加

			// マテリアル情報を直接構築してpush_back
			g_materials.emplace_back(MATERIAL{
				std::move(mtrlname),
				ambient,
				diffuse,
				specular,
				emission,
				shiness,
				std::move(texturenameToSave)
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
#ifdef _DEBUG
			std::cout << "load error" << filename.c_str() << importer.GetErrorString() << std::endl;
#endif
		}
		assert(pScene != nullptr);

		// マテリアル情報取得
		GetMaterialData(pScene, texturedirectory, filename);

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


	void GetModelDataSkinned(const std::string& filename,
		const std::string& texturedirectory)
	{
		// 既存データをクリア
		g_vertices.clear();          // 静的用は今回は使わないが念のためクリア
		g_indices.clear();
		g_subsets.clear();
		g_materials.clear();
		g_textures.clear();
		g_verticesSkinned.clear();
		g_skeleton = Skeleton{};
		g_clips.clear();

		g_sceneMin = DirectX::SimpleMath::Vector3(FLT_MAX, FLT_MAX, FLT_MAX);
		g_sceneMax = DirectX::SimpleMath::Vector3(-FLT_MAX, -FLT_MAX, -FLT_MAX);

		// Assimp でシーン読み込み
		Assimp::Importer importer;
		const aiScene* pScene = importer.ReadFile(
			filename.c_str(),
			aiProcess_ConvertToLeftHanded |
			aiProcess_Triangulate |
			aiProcess_LimitBoneWeights
		);

		if (!pScene)
		{
#ifdef _DEBUG
			std::cout << "load error (skinned): " << filename
				<< " : " << importer.GetErrorString() << std::endl;
#endif
			assert(pScene && "Assimp load failed in GetModelDataSkinned");
			return;
		}

		// マテリアル・テクスチャ情報を構築
		GetMaterialData(pScene, texturedirectory, filename);

		// スケルトン構築
		std::unordered_map<std::string, int> boneIndexMap;
		BuildSkeletonFromScene(pScene, g_skeleton, boneIndexMap);

		// 頂点 / インデックス / サブセット構築
		BuildSkinnedMeshData(pScene, g_skeleton, boneIndexMap);

		// アニメーションクリップ構築
		BuildAnimationClipsFromScene(pScene, g_skeleton, g_clips);
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

	const std::vector<std::vector<VERTEX_SKINNED>>& GetSkinnedVertices()
	{
		return g_verticesSkinned;
	}

	const Skeleton& GetSkeleton()
	{
		return g_skeleton;
	}

	const AnimationClip& GetAnimationClip(std::size_t index)
	{
		assert(index < g_clips.size());
		return g_clips[index];
	}
}