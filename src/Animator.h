#pragma once
#include "SkinnedMesh.h"

struct Skeleton;
struct AnimationClip;

class Animator
{

public:

    Animator(const Skeleton* skeleton, const AnimationClip* clip);

    void Update(float deltaTime);

    const std::vector<DirectX::SimpleMath::Matrix>&
        GetFinalMatrices() const { return m_finalMatrices; }

private:

    void CalcLocalPose(float time);
    void CalcFinalPose();

private:

    const Skeleton* m_skeleton;
    const AnimationClip* m_clip;

    float m_currentTime = 0.0f;

    std::vector<DirectX::SimpleMath::Matrix> m_localPose;
    std::vector<DirectX::SimpleMath::Matrix> m_finalMatrices;
};
