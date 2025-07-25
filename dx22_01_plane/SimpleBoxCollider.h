#pragma once
#include <SimpleMath.h>
#include"Camera.h"

struct ID3D11Device;
struct ID3D11DeviceContext;

class SimpleBoxCollider {

public:
    DirectX::SimpleMath::Vector3 center;   // ボックスの中心位置
    DirectX::SimpleMath::Vector3 size;     // ボックスのサイズ

    SimpleBoxCollider(const DirectX::SimpleMath::Vector3& center, const DirectX::SimpleMath::Vector3& size)
        : center(center), size(size) {}

    bool CheckCollision(const SimpleBoxCollider& other) const;// 当たり判定チェック (AABB方式)

    void DrawDebugCollider(const Camera& cam) const;// デバッグ用のコライダービジュアルを描画する関数

    static void InitDebugDraw(ID3D11Device* device, ID3D11DeviceContext* ctx);// グローバル初期化関数の宣言
};
