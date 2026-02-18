#pragma once

#include "Camera.h"

// 前方宣言
namespace DirectX {
    template<class VertexType> class PrimitiveBatch;
    struct VertexPositionColor;
    class BasicEffect;
    class CommonStates;
}

class DebugRenderer
{
private:
    // リソース（全オブジェクトで共有してメモリ節約）
    static std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>> m_Batch;
    static std::unique_ptr<DirectX::BasicEffect> m_Effect;
    static std::unique_ptr<DirectX::CommonStates> m_States;
    static Microsoft::WRL::ComPtr<ID3D11InputLayout> m_InputLayout;
    static bool m_Initialized;

    // ステート保存用
    static ID3D11RasterizerState* m_PrevRasterState;
    static ID3D11DepthStencilState* m_PrevDepthState;
    static UINT m_PrevStencilRef;
    static ID3D11InputLayout* m_PrevInputLayout;
    static D3D11_PRIMITIVE_TOPOLOGY m_PrevTopology;

public:
    // 初期化・終了
    static void Init(ID3D11Device* device, ID3D11DeviceContext* context);
    static void Uninit();

    // 描画開始・終了（Enemy::Drawなどで呼ぶ）
    static void Begin(const Camera& camera);
    static void End();

    // 線・形状の描画関数
    static void DrawLine(const DirectX::SimpleMath::Vector3& p1, const DirectX::SimpleMath::Vector3& p2, const DirectX::SimpleMath::Color& color);
    static void DrawRay(const DirectX::SimpleMath::Vector3& origin, const DirectX::SimpleMath::Vector3& dir, float length, const DirectX::SimpleMath::Color& color);
    static void DrawBox(const DirectX::SimpleMath::Matrix& world, const DirectX::SimpleMath::Vector3& size, const DirectX::SimpleMath::Color& color);
};