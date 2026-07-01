#include "pch.h"
#include "SimpleBoxCollider.h"
#include"Renderer.h"
#include "Camera.h"
#include "dx11helper.h"


using namespace DirectX;
using namespace DirectX::SimpleMath;
using Microsoft::WRL::ComPtr;

//変数の定義
std::unique_ptr<PrimitiveBatch<VertexPositionColor>> SimpleBoxCollider::m_Batch;
std::unique_ptr<BasicEffect> SimpleBoxCollider::m_Effect;
std::unique_ptr<CommonStates> SimpleBoxCollider::m_States;
ComPtr<ID3D11InputLayout> SimpleBoxCollider::m_InputLayout;
bool SimpleBoxCollider::m_initialized = false;



SimpleBoxCollider::SimpleBoxCollider(const DirectX::SimpleMath::Vector3& center, const DirectX::SimpleMath::Vector3& size)
	: center(center), size(size), rotation(DirectX::SimpleMath::Quaternion::Identity)
{
}

DirectX::BoundingBox SimpleBoxCollider::ToBoundingBox() const
{
	return DirectX::BoundingBox(center, size * 0.5f);
}

DirectX::BoundingOrientedBox SimpleBoxCollider::ToBoundingOrientedBox() const
{
	// Extentsはサイズの半分になるため 0.5倍 する
	return DirectX::BoundingOrientedBox(center, size * 0.5f, rotation);
}

bool SimpleBoxCollider::CheckCollision(const SimpleBoxCollider& other) const
{
	// OBB同士の衝突判定を行うため、OBBに変換してからIntersect関数を呼び出す
	DirectX::BoundingOrientedBox obb1 = ToBoundingOrientedBox();
	DirectX::BoundingOrientedBox obb2 = other.ToBoundingOrientedBox();
	return obb1.Intersects(obb2);
}



// デバッグ用のコライダービジュアルを描画する関数の初期化
void SimpleBoxCollider::InitDebugDraw(ID3D11Device* device, ID3D11DeviceContext* ctx)
{
#ifdef _DEBUG
	std::cout << "SimpleBoxCollider::InitDebugDraw()" << std::endl;
#endif
	if (m_initialized) return; // 既に初期化済みなら何もしない

	m_Batch = std::make_unique<PrimitiveBatch<VertexPositionColor>>(ctx);
	m_Effect = std::make_unique<BasicEffect>(device);
	m_Effect->SetVertexColorEnabled(true);
	m_States = std::make_unique<CommonStates>(device);

	// 入力レイアウトの作成
	void const* bytecode = nullptr;
	size_t      length = 0;
	m_Effect->GetVertexShaderBytecode(&bytecode, &length);

	ComPtr<ID3D11InputLayout> il;
	HRESULT hr = device->CreateInputLayout(
		VertexPositionColor::InputElements,
		VertexPositionColor::InputElementCount,
		bytecode,
		length,
		il.GetAddressOf()
	);
	if (SUCCEEDED(hr)) {
		m_InputLayout = il;

#ifdef _DEBUG
		std::cout << "SimpleBoxColliderの初期化:成功" << std::endl;
#endif
	}
	else {
		std::cerr << "SimpleBoxColliderの初期化:失敗" << std::endl;
	}

	m_initialized = true;
}



// デバッグ用のコライダービジュアルを描画する関数
void SimpleBoxCollider::DrawDebugCollider(const Camera& cam) const
{
	// コライダー自身が中心座標と回転を持っているため、ワールド行列を自己計算する
	Matrix world = Matrix::CreateFromQuaternion(rotation) * Matrix::CreateTranslation(center);

	// 初期化チェック
	if (!m_Batch || !m_Effect || !m_States) {
		return;
	}

	auto context = Renderer::GetDeviceContext();
	auto& effect = *m_Effect;
	auto& batch = *m_Batch;

	// --- 描画ステートの保存 ---
	D3D11DebugState saved{};
	SaveD3D11DebugState(context, saved);


	// --- デバッグ描画の開始 ---
	effect.SetWorld(world);
	effect.SetView(cam.GetViewMatrix());
	effect.SetProjection(cam.GetProjectionMatrix());
	effect.Apply(context);

	context->IASetInputLayout(m_InputLayout.Get());
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	context->OMSetDepthStencilState(m_States->DepthNone(), 0);

	batch.Begin();

	float hx = size.x * 0.5f;
	float hy = size.y * 0.5f;
	float hz = size.z * 0.5f;

	Vector3 corners[8] = {
		 Vector3(-hx, -hy, -hz),  Vector3(-hx, +hy, -hz),
		 Vector3(+hx, +hy, -hz),  Vector3(+hx, -hy, -hz),
		 Vector3(-hx, -hy, +hz),  Vector3(-hx, +hy, +hz),
		 Vector3(+hx, +hy, +hz),  Vector3(+hx, -hy, +hz)
	};

	// 12エッジを描画
	batch.DrawLine(VertexPositionColor(corners[0], Colors::Red), VertexPositionColor(corners[1], Colors::Red));
	batch.DrawLine(VertexPositionColor(corners[1], Colors::Red), VertexPositionColor(corners[2], Colors::Red));
	batch.DrawLine(VertexPositionColor(corners[2], Colors::Red), VertexPositionColor(corners[3], Colors::Red));
	batch.DrawLine(VertexPositionColor(corners[3], Colors::Red), VertexPositionColor(corners[0], Colors::Red));
	batch.DrawLine(VertexPositionColor(corners[4], Colors::Red), VertexPositionColor(corners[5], Colors::Red));
	batch.DrawLine(VertexPositionColor(corners[5], Colors::Red), VertexPositionColor(corners[6], Colors::Red));
	batch.DrawLine(VertexPositionColor(corners[6], Colors::Red), VertexPositionColor(corners[7], Colors::Red));
	batch.DrawLine(VertexPositionColor(corners[7], Colors::Red), VertexPositionColor(corners[4], Colors::Red));
	batch.DrawLine(VertexPositionColor(corners[0], Colors::Red), VertexPositionColor(corners[4], Colors::Red));
	batch.DrawLine(VertexPositionColor(corners[1], Colors::Red), VertexPositionColor(corners[5], Colors::Red));
	batch.DrawLine(VertexPositionColor(corners[2], Colors::Red), VertexPositionColor(corners[6], Colors::Red));
	batch.DrawLine(VertexPositionColor(corners[3], Colors::Red), VertexPositionColor(corners[7], Colors::Red));

	batch.End();

	// --- 描画ステートの復元 ---
	RestoreD3D11DebugState(context, saved);
}


