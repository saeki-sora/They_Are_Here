#pragma once

#include"Camera.h"

// 前方宣言
namespace DirectX
{
    template<class VertexType> class PrimitiveBatch;
    struct VertexPositionColor;
    class BasicEffect;
    class CommonStates;
}

struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11InputLayout;

class SimpleBoxCollider
{

public:

    static std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>> m_Batch;
    static std::unique_ptr<DirectX::BasicEffect>                                   m_Effect;
    static std::unique_ptr<DirectX::CommonStates>								  m_States;

    static bool m_initialized;// 初期化フラグ

    static Microsoft::WRL::ComPtr<ID3D11InputLayout> m_InputLayout;

    DirectX::SimpleMath::Vector3 center;   // ボックスの中心位置
    DirectX::SimpleMath::Vector3 size;     // ボックスのサイズ
    DirectX::SimpleMath::Quaternion rotation; // ボックスの回転 (OBB用)

    SimpleBoxCollider(const DirectX::SimpleMath::Vector3& center, const DirectX::SimpleMath::Vector3& size);

    bool CheckCollision(const SimpleBoxCollider& other) const;// 当たり判定チェック (AABB方式)

    DirectX::BoundingBox ToBoundingBox() const; // 回転を考慮しない最大内包AABBが必要な場合用
    DirectX::BoundingOrientedBox ToBoundingOrientedBox() const; // OBB作成

    void DrawDebugCollider(const Camera& cam) const;// デバッグ用のコライダービジュアルを描画する関数

    static void InitDebugDraw(ID3D11Device* device, ID3D11DeviceContext* ctx);// グローバル初期化関数の宣言
};
