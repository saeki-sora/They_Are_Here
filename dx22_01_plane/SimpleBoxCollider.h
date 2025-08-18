#pragma once
#include <SimpleMath.h>
#include <memory>
#include <wrl/client.h>

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

private:

    static std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>> m_Batch;
    static std::unique_ptr<DirectX::BasicEffect>                                   m_Effect;
    static std::unique_ptr<DirectX::CommonStates>								  m_States;

    static bool m_initialized;// 初期化フラグ

    static Microsoft::WRL::ComPtr<ID3D11InputLayout> m_InputLayout;

public:
    DirectX::SimpleMath::Vector3 center;   // ボックスの中心位置
    DirectX::SimpleMath::Vector3 size;     // ボックスのサイズ

    SimpleBoxCollider(const DirectX::SimpleMath::Vector3& center, const DirectX::SimpleMath::Vector3& size)
        : center(center), size(size) {}

    bool CheckCollision(const SimpleBoxCollider& other) const;// 当たり判定チェック (AABB方式)

    void DrawDebugCollider(const Camera& cam, const DirectX::SimpleMath::Matrix& world) const;// デバッグ用のコライダービジュアルを描画する関数
    //void DrawDebugCollider(const Camera& cam) const;// デバッグ用のコライダービジュアルを描画する関数

    static void InitDebugDraw(ID3D11Device* device, ID3D11DeviceContext* ctx);// グローバル初期化関数の宣言
};
