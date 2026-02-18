#include "pch.h"
#include	"StaticMesh.h"
#include	"AssimpPerse.h"
#include "TextureManager.h"


void StaticMesh::SaveBinary(const std::string& filename)
{
	std::ofstream ofs(filename, std::ios::binary);
	if (!ofs) return;// ファイルが開けなかった場合は何もしない

	// --- 基本情報 (min, max) ---
	ofs.write(reinterpret_cast<const char*>(&m_min), sizeof(m_min));
	ofs.write(reinterpret_cast<const char*>(&m_max), sizeof(m_max));

	// --- ベクターのサイズ情報 ---
	size_t numVertices = m_vertices.size();
	size_t numIndices = m_indices.size();
	size_t numMaterials = m_materials.size();
	size_t numSubsets = m_subsets.size();
	size_t numTextureNames = m_texturenames.size();

	ofs.write(reinterpret_cast<const char*>(&numVertices), sizeof(size_t));
	ofs.write(reinterpret_cast<const char*>(&numIndices), sizeof(size_t));
	ofs.write(reinterpret_cast<const char*>(&numMaterials), sizeof(size_t));
	ofs.write(reinterpret_cast<const char*>(&numSubsets), sizeof(size_t));
	ofs.write(reinterpret_cast<const char*>(&numTextureNames), sizeof(size_t));


	// 頂点データ
	ofs.write(reinterpret_cast<const char*>(m_vertices.data()), sizeof(VERTEX_3D) * numVertices);
	// インデックスデータ
	ofs.write(reinterpret_cast<const char*>(m_indices.data()), sizeof(unsigned int) * numIndices);
	// マテリアルデータ
	ofs.write(reinterpret_cast<const char*>(m_materials.data()), sizeof(MATERIAL) * numMaterials);

	for (const auto& subset : m_subsets) 
	{
		// まずは固定長のメンバーを書き込む
		ofs.write(reinterpret_cast<const char*>(&subset.IndexNum), sizeof(unsigned int));
		ofs.write(reinterpret_cast<const char*>(&subset.VertexNum), sizeof(unsigned int));
		ofs.write(reinterpret_cast<const char*>(&subset.IndexBase), sizeof(unsigned int));
		ofs.write(reinterpret_cast<const char*>(&subset.VertexBase), sizeof(unsigned int));
		ofs.write(reinterpret_cast<const char*>(&subset.MaterialIdx), sizeof(unsigned int));

		// 次に可変長メンバー (MtrlName) を「長さ + データ本体」の形式で書き込む
		uint16_t nameLength = static_cast<uint16_t>(subset.MtrlName.length());
		ofs.write(reinterpret_cast<const char*>(&nameLength), sizeof(uint16_t));
		ofs.write(subset.MtrlName.c_str(), nameLength);
	}

	// テクスチャ名 (可変長なので１つずつ)
	for (const auto& name : m_texturenames)
	{
		uint16_t nameLength = static_cast<uint16_t>(name.length());

		//文字列の長さを書き込む
		ofs.write(reinterpret_cast<const char*>(&nameLength), sizeof(uint16_t));

		//文字列のデータ本体を書き込む
		ofs.write(name.c_str(), nameLength);
	}

	ofs.close();
}


bool StaticMesh::LoadBinary(const std::string& filename)
{
	std::ifstream ifs(filename, std::ios::binary);
	if (!ifs) {
		// ファイルが開けない or 存在しない
		return false;
	}

	// --- 基本情報 (min, max) ---
	ifs.read(reinterpret_cast<char*>(&m_min), sizeof(m_min));
	ifs.read(reinterpret_cast<char*>(&m_max), sizeof(m_max));

	// --- ベクターのサイズ情報 ---
	size_t numVertices = 0;
	size_t numIndices = 0;
	size_t numMaterials = 0;
	size_t numSubsets = 0;
	size_t numTextureNames = 0;

	ifs.read(reinterpret_cast<char*>(&numVertices), sizeof(size_t));
	ifs.read(reinterpret_cast<char*>(&numIndices), sizeof(size_t));
	ifs.read(reinterpret_cast<char*>(&numMaterials), sizeof(size_t));
	ifs.read(reinterpret_cast<char*>(&numSubsets), sizeof(size_t));
	ifs.read(reinterpret_cast<char*>(&numTextureNames), sizeof(size_t));

	// --- データ本体 ---
	// 読み込む前に vector のサイズを確保
	m_vertices.resize(numVertices);
	m_indices.resize(numIndices);
	m_materials.resize(numMaterials);
	m_subsets.resize(numSubsets);
	m_texturenames.resize(numTextureNames);

	// 頂点データ
	ifs.read(reinterpret_cast<char*>(m_vertices.data()), sizeof(VERTEX_3D) * numVertices);
	// インデックスデータ
	ifs.read(reinterpret_cast<char*>(m_indices.data()), sizeof(unsigned int) * numIndices);
	// マテリアルデータ
	ifs.read(reinterpret_cast<char*>(m_materials.data()), sizeof(MATERIAL) * numMaterials);

	for (size_t i = 0; i < numSubsets; ++i) 
	{
		// まずは固定長のメンバーを読み込む
		ifs.read(reinterpret_cast<char*>(&m_subsets[i].IndexNum), sizeof(unsigned int));
		ifs.read(reinterpret_cast<char*>(&m_subsets[i].VertexNum), sizeof(unsigned int));
		ifs.read(reinterpret_cast<char*>(&m_subsets[i].IndexBase), sizeof(unsigned int));
		ifs.read(reinterpret_cast<char*>(&m_subsets[i].VertexBase), sizeof(unsigned int));
		ifs.read(reinterpret_cast<char*>(&m_subsets[i].MaterialIdx), sizeof(unsigned int));

		// 次に可変長メンバーの順で読み込む
		uint16_t  nameLength = 0;
		ifs.read(reinterpret_cast<char*>(&nameLength), sizeof(uint16_t));
		m_subsets[i].MtrlName.resize(nameLength);
		ifs.read(&m_subsets[i].MtrlName[0], nameLength);
	}

	// テクスチャ名 (可変長なので１つずつ)
	for (size_t i = 0; i < numTextureNames; ++i) 
	{
		uint16_t  nameLength = 0;

		//文字列の長さを読み込む
		ifs.read(reinterpret_cast<char*>(&nameLength), sizeof(uint16_t));

		//文字列のデータ本体を読み込む
		//一時的なバッファを用意して読み込む
		std::vector<char> buf(nameLength);
		ifs.read(buf.data(), nameLength);
		m_texturenames[i] = std::string(buf.begin(), buf.end());
	}

	ifs.close();
	return true; // 読み込み成功
}



void StaticMesh::Load(std::string filename, std::string texturedirectory)
{
	// バイナリファイルのパスを作成
	std::string binaryFilename = filename;
	size_t dotPos = binaryFilename.rfind('.');

	if (dotPos != std::string::npos)
	{
		binaryFilename.replace(dotPos, binaryFilename.length() - dotPos, ".bin");
	}

	// バイナリファイルの読み込みを試みる
	if (LoadBinary(binaryFilename)) {

		m_textures.resize(m_texturenames.size());

		for (size_t i = 0; i < m_texturenames.size(); ++i) 
		{
			const auto & name = m_texturenames[i];
			if (!name.empty())
			{
				std::filesystem::path texPath = std::filesystem::path(texturedirectory) / name;
				m_textures[i] = TextureManager::GetInstance().Load(texPath.lexically_normal().string());
				
			}
			else {
				m_textures[i] = TextureManager::GetInstance().Load("");
				
			}
			
		}
		return; // 成功したら終了
	}

	if (m_textures.size() < m_materials.size()) 
	{
		m_textures.resize(m_materials.size(), TextureManager::GetInstance().Load(""));
	}
	else if (m_textures.size() > m_materials.size()) {
		m_textures.resize(m_materials.size());
	}


	// ==========================================================
	//バイナリファイルがなかった場合
	// ==========================================================
	std::vector<AssimpPerse::SUBSET> subsets{};					// サブセット情報
	std::vector<std::vector<AssimpPerse::VERTEX>> vertices{};	// 頂点データ（メッシュ単位）
	std::vector<std::vector<unsigned int>> indices{};			// インデックスデータ（メッシュ単位）
	std::vector<AssimpPerse::MATERIAL> materials{};				// マテリアル
	std::vector<std::unique_ptr<Texture>> embededtextures{};	// 内蔵テクスチャ群

	// assimpを使用してモデルデータを取得
	AssimpPerse::GetModelData(filename, texturedirectory);

	subsets = AssimpPerse::GetSubsets();		// サブセット情報取得
	vertices = AssimpPerse::GetVertices();		// 頂点データ（メッシュ単位）
	indices = AssimpPerse::GetIndices();		// インデックスデータ（メッシュ単位）
	materials = AssimpPerse::GetMaterials();	// マテリアル情報取得

	m_textures = AssimpPerse::GetTextures();	// テクスチャ情報取得	

	// 頂点データ作成
	for (const auto& mv : vertices)
	{
		for (auto& v : mv)
		{
			VERTEX_3D vertex{};
			vertex.position = DirectX::SimpleMath::Vector3(v.pos.x, v.pos.y, v.pos.z);
			vertex.normal = DirectX::SimpleMath::Vector3(v.normal.x, v.normal.y, v.normal.z);
			vertex.uv = DirectX::SimpleMath::Vector2(v.texcoord.x, v.texcoord.y);
			vertex.color = DirectX::SimpleMath::Color(v.color.r, v.color.g, v.color.b, v.color.a);

			m_vertices.emplace_back(vertex);
		}
	}

	// インデックスデータ作成
	for (const auto& mi : indices)
	{
		for (auto& index : mi)
		{
			m_indices.emplace_back(index);
		}
	}

	// サブセットデータ作成
	for (const auto& sub : subsets)
	{
		SUBSET subset{};
		subset.VertexBase = sub.VertexBase; // 頂点の開始位置
		subset.VertexNum = sub.VertexNum; // サブセット内の頂点数
		subset.IndexBase = sub.IndexBase;  // インデックスの開始位置
		subset.IndexNum = sub.IndexNum; // サブセット内のインデックス数
		subset.MtrlName = sub.mtrlname; // マテリアル名
		subset.MaterialIdx = sub.materialindex; // マテリアル配列のインデックス
		m_subsets.emplace_back(subset);
	}

	//マテリアルデータ作成
	for (const auto& m : materials)
	{
		MATERIAL material{};
		material.Ambient = DirectX::SimpleMath::Color(m.Ambient.r, m.Ambient.g, m.Ambient.b, m.Ambient.a);
		material.Diffuse = DirectX::SimpleMath::Color(m.Diffuse.r, m.Diffuse.g, m.Diffuse.b, m.Diffuse.a);

		material.Specular = DirectX::SimpleMath::Color(m.Specular.r, m.Specular.g, m.Specular.b, m.Specular.a);
		material.Emission = DirectX::SimpleMath::Color(m.Emission.r, m.Emission.g, m.Emission.b, m.Emission.a);
		material.Shiness = m.Shiness;

		if (m.texturename.empty())
		{
			material.TextureEnable = false;
			m_texturenames.emplace_back("");
		}
		else
		{
			material.TextureEnable = true;
			m_texturenames.emplace_back(m.texturename);
		}

		m_materials.emplace_back(material);
	}

	m_min = AssimpPerse::GetSceneMin(); // AssimpPerseから最小座標を取得
	m_max = AssimpPerse::GetSceneMax(); // AssimpPerseから最大座標を取得

	SaveBinary(binaryFilename); // バイナリファイルとして保存
}