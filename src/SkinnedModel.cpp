#include "pch.h"
#include "SkinnedModel.h"
#include "SkinnedMesh.h"
#include "Animator.h"


SkinnedModel::SkinnedModel() = default;

SkinnedModel::~SkinnedModel() = default;


bool SkinnedModel::Load(const std::string& filename, const std::string& texDir)
{
    m_mesh = std::make_unique<SkinnedMesh>();
    m_mesh->Load(filename, texDir);

    // スケルトン情報とアニメ1本を取得
    const Skeleton& skeleton = m_mesh->GetSkeleton();
    const AnimationClip& clip = m_mesh->GetAnimationClip();

    // Animator を作る（アニメ1個想定）
    m_animator = std::make_unique<Animator>(&skeleton, &clip);

    return true;
}

void SkinnedModel::Update(float dt)
{
    if (m_animator)
    {
        m_animator->Update(dt);
    }
}


const std::vector<DirectX::SimpleMath::Matrix>& SkinnedModel::GetFinalMatrices() const
{
    return m_animator->GetFinalMatrices();
}