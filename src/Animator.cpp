#include "pch.h"
#include "Animator.h"

using namespace DirectX::SimpleMath;

Animator::Animator(const Skeleton* skeleton, const AnimationClip* clip)
    : m_skeleton(skeleton), m_clip(clip)
{
    size_t boneCount = skeleton->bones.size();
    m_localPose.resize(boneCount, Matrix::Identity);
    m_finalMatrices.resize(boneCount, Matrix::Identity);
}

void Animator::Update(float dt)
{
    m_currentTime += dt * m_clip->ticksPerSecond;

    // ループ
    if (m_currentTime > m_clip->duration)
        m_currentTime = fmod(m_currentTime, m_clip->duration);

    CalcLocalPose(m_currentTime);
    CalcFinalPose();
}

void Animator::CalcLocalPose(float time)
{
    for (size_t i = 0; i < m_clip->boneAnimations.size(); ++i)
    {
        const BoneAnimation& anim = m_clip->boneAnimations[i];

        if (anim.keyframes.empty())
        {
            m_localPose[i] = Matrix::Identity;
            continue;
        }

        // 単純化：キー0の値で適用（後で補間追加）
        const Keyframe& kf = anim.keyframes[0];

        m_localPose[i] =
            Matrix::CreateScale(kf.scale) *
            Matrix::CreateFromQuaternion(kf.rotation) *
            Matrix::CreateTranslation(kf.translation);
    }
}

void Animator::CalcFinalPose()
{
    for (size_t i = 0; i < m_skeleton->bones.size(); ++i)
    {
        int parent = m_skeleton->bones[i].parentIndex;

        if (parent >= 0)
        {
            m_finalMatrices[i] =
                m_localPose[i] *
                m_finalMatrices[parent];
        }
        else
        {
            m_finalMatrices[i] = m_localPose[i];
        }

        // スキニング変換として inverseBindPose を適用
        m_finalMatrices[i] =
            m_skeleton->bones[i].inverseBindPose * m_finalMatrices[i];
    }
}
