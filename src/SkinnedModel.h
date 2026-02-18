#pragma once


class SkinnedMesh;
class Animator;

class SkinnedModel
{
private:

    std::unique_ptr<SkinnedMesh> m_mesh;
    std::unique_ptr<Animator>    m_animator;

public:

    SkinnedModel();
    ~SkinnedModel();

    // モデルとアニメを読み込み
    bool Load(const std::string& filename, const std::string& texDir = "");
    void Update(float deltaTime);

    //描画用
    const SkinnedMesh& GetMesh() const { return *m_mesh; }
    const std::vector<DirectX::SimpleMath::Matrix>& GetFinalMatrices() const;
};
