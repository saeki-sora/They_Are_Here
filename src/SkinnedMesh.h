#pragma once

#include"Mesh.h"
#include"Texture.h"


// ボーン1本分の情報
struct Bone
{
    std::string name;
    int         parentIndex = -1;

    DirectX::SimpleMath::Matrix bindPose = DirectX::SimpleMath::Matrix::Identity;
    DirectX::SimpleMath::Matrix inverseBindPose = DirectX::SimpleMath::Matrix::Identity;
};

// スケルトン（ボーンツリー）
struct Skeleton
{
    std::vector<Bone> bones;

    // ボーン名からインデックスを検索
    int FindBoneIndex(const std::string& name) const
    {
        for (std::size_t i = 0; i < bones.size(); ++i)
        {
            if (bones[i].name == name)
            {
                return static_cast<int>(i);
            }
        }
        return -1;
    }
};

// 1キー分のTRS
struct Keyframe
{
    float time = 0.0f;

    DirectX::SimpleMath::Vector3     translation = DirectX::SimpleMath::Vector3::Zero;
    DirectX::SimpleMath::Quaternion  rotation = DirectX::SimpleMath::Quaternion::Identity;
    DirectX::SimpleMath::Vector3     scale = DirectX::SimpleMath::Vector3(1.0f, 1.0f, 1.0f);
};

// 1ボーン分のアニメーション
struct BoneAnimation
{
    std::vector<Keyframe> keyframes;
};

// アニメーションクリップ（1本）
struct AnimationClip
{
    std::string name;
    float       duration = 0.0f;   // tickベースの長さ
    float       ticksPerSecond = 30.0f;

    std::vector<BoneAnimation> boneAnimations;
};


class SkinnedMesh : public Mesh
{
public:

    void Load(const std::string& filename, const std::string& textureDirectory = "");


    const Skeleton& GetSkeleton() const { return m_skeleton; }
    const AnimationClip& GetAnimationClip() const { return m_clip; }

    const std::vector<MATERIAL>& GetMaterials() const { return m_materials; }
    const std::vector<SUBSET>& GetSubsets() const { return m_subsets; }
    const std::vector<std::shared_ptr<Texture>>& GetTextures() const { return m_textures; }

    ID3D11Buffer* const* GetVertexBuffer() const { return m_vertexBuffer.GetAddressOf(); }
    ID3D11Buffer* GetIndexBuffer() const { return m_indexBuffer.Get(); }

    const DirectX::SimpleMath::Vector3& GetMin() const { return m_min; }
    const DirectX::SimpleMath::Vector3& GetMax() const { return m_max; }

protected:

    Microsoft::WRL::ComPtr<ID3D11Buffer> m_vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_indexBuffer;

    std::vector<VERTEX_SKINNED> m_verticesSkinned;

    std::vector<unsigned int> m_indices;
    std::vector<MATERIAL>     m_materials;
    std::vector<SUBSET>       m_subsets;
    std::vector<std::shared_ptr<Texture>> m_textures;

    DirectX::SimpleMath::Vector3 m_min;
    DirectX::SimpleMath::Vector3 m_max;

    Skeleton      m_skeleton;
    AnimationClip m_clip;
};
