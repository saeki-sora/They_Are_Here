#include "pch.h"
#include "SkinnedMesh.h"
#include "AssimpPerse.h"
#include "TextureManager.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

//------------------------------------------------------------
// スキニングメッシュ読み込み
//------------------------------------------------------------
void SkinnedMesh::Load(const std::string& filename,
    const std::string& textureDirectory)
{
    // 一旦クリア
    m_verticesSkinned.clear();
    m_indices.clear();
    m_materials.clear();
    m_subsets.clear();
    m_textures.clear();
    m_skeleton = Skeleton{};
    m_clip = AnimationClip{};
    m_min = Vector3::Zero;
    m_max = Vector3::Zero;

    // --------------------------------------------------------
    // Assimp でモデル＋スケルトン＋アニメを構築
    // --------------------------------------------------------
    AssimpPerse::GetModelDataSkinned(filename, textureDirectory);

    const auto& srcSkinnedVertsAll = AssimpPerse::GetSkinnedVertices();
    const auto& srcIndicesAll = AssimpPerse::GetIndices();
    const auto& srcSubsets = AssimpPerse::GetSubsets();
    const auto& srcMaterials = AssimpPerse::GetMaterials();

    // テクスチャはムーブアウトされる仕様
    auto        texList = AssimpPerse::GetTextures();

    m_min = AssimpPerse::GetSceneMin();
    m_max = AssimpPerse::GetSceneMax();

    // --------------------------------------------------------
    // 頂点／インデックスを 1 本の配列にフラット化
    // --------------------------------------------------------
    std::size_t totalVertices = 0;
    std::size_t totalIndices = 0;
    for (std::size_t m = 0; m < srcSkinnedVertsAll.size(); ++m)
    {
        totalVertices += srcSkinnedVertsAll[m].size();
        totalIndices += srcIndicesAll[m].size();
    }

    m_verticesSkinned.reserve(totalVertices);
    m_indices.reserve(totalIndices);
    m_subsets.reserve(srcSubsets.size());

    // メッシュごとにフラット化
    for (std::size_t m = 0; m < srcSkinnedVertsAll.size(); ++m)
    {
        const auto& meshVerts = srcSkinnedVertsAll[m];
        const auto& meshIdx = srcIndicesAll[m];
        const auto& srcSub = srcSubsets[m];   // AssimpPerse::SUBSET 側

        // このメッシュがフラット配列内で開始する位置
        const unsigned int baseVertex = static_cast<unsigned int>(m_verticesSkinned.size());
        const unsigned int baseIndex = static_cast<unsigned int>(m_indices.size());

        // 頂点コピー
        m_verticesSkinned.insert(
            m_verticesSkinned.end(),
            meshVerts.begin(),
            meshVerts.end());

        // インデックス（頂点のオフセットを加算）
        m_indices.reserve(m_indices.size() + meshIdx.size());
        for (unsigned int idx : meshIdx)
        {
            m_indices.push_back(baseVertex + idx);
        }

        // 実行時用 SUBSET（Renderer.h で定義されている方）を構築
        SUBSET dstSub{};
        dstSub.VertexBase = baseVertex;
        dstSub.IndexBase = baseIndex;
        dstSub.VertexNum = static_cast<unsigned int>(meshVerts.size());
        dstSub.IndexNum = static_cast<unsigned int>(meshIdx.size());
        dstSub.MaterialIdx = static_cast<unsigned int>(srcSub.materialindex);
        dstSub.MtrlName = srcSub.mtrlname;

        m_subsets.push_back(dstSub);
    }

    // --------------------------------------------------------
    // マテリアル変換
    // --------------------------------------------------------
    m_materials.reserve(srcMaterials.size());
    for (const auto& m : srcMaterials)
    {
        MATERIAL mat{};
        mat.Ambient = Color(m.Ambient.r, m.Ambient.g, m.Ambient.b, m.Ambient.a);
        mat.Diffuse = Color(m.Diffuse.r, m.Diffuse.g, m.Diffuse.b, m.Diffuse.a);
        mat.Specular = Color(m.Specular.r, m.Specular.g, m.Specular.b, m.Specular.a);
        mat.Emission = Color(m.Emission.r, m.Emission.g, m.Emission.b, m.Emission.a);

        mat.Shiness = m.Shiness;
        mat.TextureEnable = !m.texturename.empty();
        mat.Dummy[0] = FALSE;
        mat.Dummy[1] = FALSE;

        m_materials.push_back(mat);
    }

    // --------------------------------------------------------
    // テクスチャ配列を反映
    // --------------------------------------------------------
    m_textures = std::move(texList);

    if (m_textures.size() < m_materials.size())
    {
        m_textures.resize(
            m_materials.size(),
            TextureManager::GetInstance().Load(""));   // デフォルト
    }
    else if (m_textures.size() > m_materials.size())
    {
        m_textures.resize(m_materials.size());
    }

    // --------------------------------------------------------
    // スケルトン & アニメーションクリップを取得
    // --------------------------------------------------------
    m_skeleton = AssimpPerse::GetSkeleton();

    m_clip = AssimpPerse::GetAnimationClip(0);

    // --------------------------------------------------------
    // GPU バッファの生成
    // --------------------------------------------------------

    auto* device = Renderer::GetDevice();

    D3D11_BUFFER_DESC vbd{};
    vbd.Usage = D3D11_USAGE_DEFAULT;
    vbd.ByteWidth = static_cast<UINT>(sizeof(VERTEX_SKINNED) * m_verticesSkinned.size());
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA vinit{};
    vinit.pSysMem = m_verticesSkinned.data();

    device->CreateBuffer(&vbd, &vinit, &m_vertexBuffer);

    D3D11_BUFFER_DESC ibd{};
    ibd.Usage = D3D11_USAGE_DEFAULT;
    ibd.ByteWidth = static_cast<UINT>(sizeof(unsigned int) * m_indices.size());
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA iinit{};
    iinit.pSysMem = m_indices.data();

    device->CreateBuffer(&ibd, &iinit, &m_indexBuffer);

}
