#include "SimpleBoxCollider.h"
#include"Renderer.h"

#include <SimpleMath.h>
#include <DirectXColors.h>
#include <PrimitiveBatch.h>
#include <VertexTypes.h>
#include <CommonStates.h>
#include <Effects.h>

using namespace DirectX;
using namespace DirectX::SimpleMath;

// グローバル変数の宣言
std::unique_ptr<PrimitiveBatch<VertexPositionColor>> g_pBatch;
std::unique_ptr<BasicEffect>                        g_pEffect;
std::unique_ptr<CommonStates>                       g_pStates;


bool SimpleBoxCollider::CheckCollision(const SimpleBoxCollider& other) const
{

	// AABB方式の当たり判定
	return (abs(center.x - other.center.x) <= (size.x + other.size.x) * 0.5f) &&
		(abs(center.y - other.center.y) <= (size.y + other.size.y) * 0.5f) &&
		(abs(center.z - other.center.z) <= (size.z + other.size.z) * 0.5f);
}

// デバッグ用のコライダービジュアルを描画する関数の初期化
void SimpleBoxCollider::InitDebugDraw(ID3D11Device* device, ID3D11DeviceContext* ctx)
{
	if (g_pBatch) return;// 既に初期化済みなら何もしない

	g_pBatch = std::make_unique<PrimitiveBatch<VertexPositionColor>>(ctx);
	g_pEffect = std::make_unique<BasicEffect>(device);
	g_pEffect->SetVertexColorEnabled(true);
	g_pStates = std::make_unique<CommonStates>(device);
}


// デバッグ用のコライダービジュアルを描画する関数
void SimpleBoxCollider::DrawDebugCollider(const Camera& cam) const
{

	auto& effect = *g_pEffect;
	auto& batch = *g_pBatch;
	auto world = Matrix::Identity;
	effect.SetWorld(world);
	effect.SetView(cam.GetViewMatrix());
	effect.SetProjection(cam.GetProjectionMatrix());
	effect.Apply(Renderer::GetDeviceContext());

	Renderer::GetDeviceContext()->IASetInputLayout(nullptr);
	Renderer::GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	// 半分の各軸長
	float hx = size.x * 0.5f;
	float hy = size.y * 0.5f;
	float hz = size.z * 0.5f;

	// 8 つのコーナー
	Vector3 corners[8] = {
		center + Vector3(-hx, -hy, -hz),
		center + Vector3(-hx, +hy, -hz),
		center + Vector3(+hx, +hy, -hz),
		center + Vector3(+hx, -hy, -hz),
		center + Vector3(-hx, -hy, +hz),
		center + Vector3(-hx, +hy, +hz),
		center + Vector3(+hx, +hy, +hz),
		center + Vector3(+hx, -hy, +hz)
	};

	// 12 エッジを描くための頂点
	const int indices[24] = {
		0,1, 1,2, 2,3, 3,0, // 下面
		4,5, 5,6, 6,7, 7,4, // 上面
		0,4, 1,5, 2,6, 3,7  // 側面
	};

	batch.Begin();
	for (int i = 0; i < 24; ++i)
	{
		int idx = indices[i];
		batch.DrawLine(
			VertexPositionColor(corners[idx], Colors::Red),
			VertexPositionColor(corners[indices[i ^ 1]], Colors::Red)
		);
		++i;
	}

	batch.End();

}