#include "pch.h"
#include "SimpleBoxCollider.h"
#include"Renderer.h"
#include "Camera.h"


using namespace DirectX;
using namespace DirectX::SimpleMath;
using Microsoft::WRL::ComPtr;

//変数の定義
std::unique_ptr<PrimitiveBatch<VertexPositionColor>> SimpleBoxCollider::m_Batch;
std::unique_ptr<BasicEffect> SimpleBoxCollider::m_Effect;
std::unique_ptr<CommonStates> SimpleBoxCollider::m_States;
ComPtr<ID3D11InputLayout> SimpleBoxCollider::m_InputLayout;
bool SimpleBoxCollider::m_initialized = false;



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
	std::cout << "SimpleBoxCollider::InitDebugDraw()" << std::endl;
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

		std::cout << "SimpleBoxColliderの初期化:成功" << std::endl;
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

	// --- 描画ステートの完全な保存 ---
	ID3D11RasterizerState* prevRasterState = nullptr;
	ID3D11DepthStencilState* prevDepthState = nullptr;
	UINT                     prevStencilRef = 0;
	ID3D11BlendState* prevBlendState = nullptr;
	float                    prevBlendFactor[4] = { 0.0f };
	UINT                     prevSampleMask = 0;
	ID3D11InputLayout* prevInputLayout = nullptr;
	D3D11_PRIMITIVE_TOPOLOGY prevTopology;
	ID3D11VertexShader* prevVS = nullptr;
	ID3D11ClassInstance* prevVSClassInstances[256] = { nullptr };
	UINT                     numVSClassInstances = 256;
	ID3D11PixelShader* prevPS = nullptr;
	ID3D11ClassInstance* prevPSClassInstances[256] = { nullptr };
	UINT                     numPSClassInstances = 256;

	context->RSGetState(&prevRasterState);
	context->OMGetDepthStencilState(&prevDepthState, &prevStencilRef);
	context->OMGetBlendState(&prevBlendState, prevBlendFactor, &prevSampleMask);
	context->IAGetInputLayout(&prevInputLayout);
	context->IAGetPrimitiveTopology(&prevTopology);
	context->VSGetShader(&prevVS, prevVSClassInstances, &numVSClassInstances);
	context->PSGetShader(&prevPS, prevPSClassInstances, &numPSClassInstances);

	// === IA VB/IB の保存 ===
	ID3D11Buffer* prevVB[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT] = {};
	UINT prevStride[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT] = {};
	UINT prevOffset[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT] = {};
	context->IAGetVertexBuffers(0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT,
		prevVB, prevStride, prevOffset);

	ID3D11Buffer* prevIB = nullptr;
	DXGI_FORMAT   prevIBFmt = DXGI_FORMAT_UNKNOWN;
	UINT          prevIBOfs = 0;
	context->IAGetIndexBuffer(&prevIB, &prevIBFmt, &prevIBOfs);

	// ===VS/PS の CB 保存 ===
	ID3D11Buffer* prevVSCB[4] = {};
	ID3D11Buffer* prevPSCB[4] = {};
	context->VSGetConstantBuffers(0, 4, prevVSCB);
	context->PSGetConstantBuffers(0, 4, prevPSCB);

	// === PS の SRV/Sampler 保存 ===
	ID3D11ShaderResourceView* prevPSRV[4] = {};
	ID3D11SamplerState* prevPSSamp[4] = {};
	context->PSGetShaderResources(0, 4, prevPSRV);
	context->PSGetSamplers(0, 4, prevPSSamp);


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

	// === IA VB/IB を復元 ===
	context->IASetVertexBuffers(0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT,
		prevVB, prevStride, prevOffset);
	context->IASetIndexBuffer(prevIB, prevIBFmt, prevIBOfs);

	// 参照カウント解放
	for (auto* b : prevVB) if (b) b->Release();
	if (prevIB) prevIB->Release();

	// === VS/PS の CB 復元 ===
	context->VSSetConstantBuffers(0, 4, prevVSCB);
	context->PSSetConstantBuffers(0, 4, prevPSCB);
	for (auto* b : prevVSCB) if (b) b->Release();
	for (auto* b : prevPSCB) if (b) b->Release();

	// === PS の SRV/Sampler 復元 ===
	context->PSSetShaderResources(0, 4, prevPSRV);
	context->PSSetSamplers(0, 4, prevPSSamp);
	for (auto* v : prevPSRV)   if (v) v->Release();
	for (auto* s : prevPSSamp) if (s) s->Release();


	// --- 描画ステートの完全な復元 ---
	context->RSSetState(prevRasterState);
	context->OMSetDepthStencilState(prevDepthState, prevStencilRef);
	context->OMSetBlendState(prevBlendState, prevBlendFactor, prevSampleMask);
	context->IASetInputLayout(prevInputLayout);
	context->IASetPrimitiveTopology(prevTopology);
	context->VSSetShader(prevVS, prevVSClassInstances, numVSClassInstances);
	context->PSSetShader(prevPS, prevPSClassInstances, numPSClassInstances);

	// --- 取得したインターフェースの解放 ---
	if (prevRasterState) prevRasterState->Release();
	if (prevDepthState) prevDepthState->Release();
	if (prevBlendState) prevBlendState->Release();
	if (prevInputLayout) prevInputLayout->Release();
	if (prevVS) prevVS->Release();
	for (UINT i = 0; i < numVSClassInstances; ++i) { if (prevVSClassInstances[i]) prevVSClassInstances[i]->Release(); }
	if (prevPS) prevPS->Release();
	for (UINT i = 0; i < numPSClassInstances; ++i) { if (prevPSClassInstances[i]) prevPSClassInstances[i]->Release(); }

}


