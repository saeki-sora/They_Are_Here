#pragma once
#include "VisualObject.h"

class SkyDome : public VisualObject
{
public:
    SkyDome(
        const DirectX::SimpleMath::Vector3& pos = { 0.0f, 0.0f, 0.0f },
        const DirectX::SimpleMath::Vector3& size = { 1.0f, 1.0f, 1.0f }
    );
    virtual ~SkyDome() {}

    void Init() override;
    void Update(float deltaTime) override;
    void Draw() override;
    bool Is3D() const override;

private:

    Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_rasterizerState;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> m_depthStencilState;

    UINT m_indexCount = 0;
};

