#include "pch.h"
#include "DebugRenderer.h"
#include "Renderer.h" 


using namespace DirectX;
using namespace DirectX::SimpleMath;

// 静的メンバの実体定義
std::unique_ptr<PrimitiveBatch<VertexPositionColor>> DebugRenderer::m_Batch;
std::unique_ptr<BasicEffect> DebugRenderer::m_Effect;
std::unique_ptr<CommonStates> DebugRenderer::m_States;
Microsoft::WRL::ComPtr<ID3D11InputLayout> DebugRenderer::m_InputLayout;
bool DebugRenderer::m_Initialized = false;

ID3D11RasterizerState* DebugRenderer::m_PrevRasterState = nullptr;
ID3D11DepthStencilState* DebugRenderer::m_PrevDepthState = nullptr;
UINT DebugRenderer::m_PrevStencilRef = 0;
ID3D11InputLayout* DebugRenderer::m_PrevInputLayout = nullptr;
D3D11_PRIMITIVE_TOPOLOGY DebugRenderer::m_PrevTopology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;


void DebugRenderer::Init(ID3D11Device* device, ID3D11DeviceContext* context)
{
    if (m_Initialized) return;

    m_States = std::make_unique<CommonStates>(device);
    m_Batch = std::make_unique<PrimitiveBatch<VertexPositionColor>>(context);
    m_Effect = std::make_unique<BasicEffect>(device);
    m_Effect->SetVertexColorEnabled(true);

    void const* shaderByteCode;
    size_t byteCodeLength;
    m_Effect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);

    device->CreateInputLayout(VertexPositionColor::InputElements,
        VertexPositionColor::InputElementCount,
        shaderByteCode, byteCodeLength,
        m_InputLayout.GetAddressOf());

    m_Initialized = true;
}

void DebugRenderer::Uninit()
{
    m_Batch.reset();
    m_Effect.reset();
    m_States.reset();
    m_InputLayout.Reset();
    m_Initialized = false;
}

void DebugRenderer::Begin(const Camera& camera)
{
    if (!m_Initialized) return;

    auto context = Renderer::GetDeviceContext();

    // --- 現在のステートを保存 (必要最低限) ---
    context->RSGetState(&m_PrevRasterState);
    context->OMGetDepthStencilState(&m_PrevDepthState, &m_PrevStencilRef);
    context->IAGetInputLayout(&m_PrevInputLayout);
    context->IAGetPrimitiveTopology(&m_PrevTopology);

    // --- デバッグ描画用ステート設定 ---
    context->OMSetDepthStencilState(m_States->DepthNone(), 0); // 常に手前に表示
    context->IASetInputLayout(m_InputLayout.Get());

    m_Effect->SetView(camera.GetViewMatrix());
    m_Effect->SetProjection(camera.GetProjectionMatrix());
    m_Effect->Apply(context);

    m_Batch->Begin();
}

void DebugRenderer::End()
{
    if (!m_Initialized) return;

    m_Batch->End();

    auto context = Renderer::GetDeviceContext();

    // --- ステート復元 ---
    context->RSSetState(m_PrevRasterState);
    context->OMSetDepthStencilState(m_PrevDepthState, m_PrevStencilRef);
    context->IASetInputLayout(m_PrevInputLayout);
    context->IASetPrimitiveTopology(m_PrevTopology);

    // 参照カウント解放
    if (m_PrevRasterState) { m_PrevRasterState->Release(); m_PrevRasterState = nullptr; }
    if (m_PrevDepthState) { m_PrevDepthState->Release(); m_PrevDepthState = nullptr; }
    if (m_PrevInputLayout) { m_PrevInputLayout->Release(); m_PrevInputLayout = nullptr; }
}

void DebugRenderer::DrawLine(const Vector3& p1, const Vector3& p2, const Color& color)
{
    if (!m_Initialized) return;
    m_Batch->DrawLine(VertexPositionColor(p1, color), VertexPositionColor(p2, color));
}

void DebugRenderer::DrawRay(const Vector3& origin, const Vector3& dir, float length, const Color& color)
{
    DrawLine(origin, origin + (dir * length), color);
}

void DebugRenderer::DrawBox(const Matrix& world, const Vector3& size, const Color& color)
{
    if (!m_Initialized) return;

    // エフェクトにワールド行列をセット（これで回転などが反映される）
    m_Effect->SetWorld(world);
    m_Effect->Apply(Renderer::GetDeviceContext());

    // ローカル座標系での8頂点計算
    Vector3 h = size * 0.5f;
    Vector3 p[8];
    p[0] = Vector3(-h.x, -h.y, -h.z); p[1] = Vector3(h.x, -h.y, -h.z);
    p[2] = Vector3(h.x, h.y, -h.z);   p[3] = Vector3(-h.x, h.y, -h.z);
    p[4] = Vector3(-h.x, -h.y, h.z);  p[5] = Vector3(h.x, -h.y, h.z);
    p[6] = Vector3(h.x, h.y, h.z);    p[7] = Vector3(-h.x, h.y, h.z);

    // 線を描画
    m_Batch->DrawLine(VertexPositionColor(p[0], color), VertexPositionColor(p[1], color));
    m_Batch->DrawLine(VertexPositionColor(p[1], color), VertexPositionColor(p[5], color));
    m_Batch->DrawLine(VertexPositionColor(p[5], color), VertexPositionColor(p[4], color));
    m_Batch->DrawLine(VertexPositionColor(p[4], color), VertexPositionColor(p[0], color));

    m_Batch->DrawLine(VertexPositionColor(p[3], color), VertexPositionColor(p[2], color));
    m_Batch->DrawLine(VertexPositionColor(p[2], color), VertexPositionColor(p[6], color));
    m_Batch->DrawLine(VertexPositionColor(p[6], color), VertexPositionColor(p[7], color));
    m_Batch->DrawLine(VertexPositionColor(p[7], color), VertexPositionColor(p[3], color));

    m_Batch->DrawLine(VertexPositionColor(p[0], color), VertexPositionColor(p[3], color));
    m_Batch->DrawLine(VertexPositionColor(p[1], color), VertexPositionColor(p[2], color));
    m_Batch->DrawLine(VertexPositionColor(p[5], color), VertexPositionColor(p[6], color));
    m_Batch->DrawLine(VertexPositionColor(p[4], color), VertexPositionColor(p[7], color));

    // ワールド行列を戻しておく
    m_Effect->SetWorld(Matrix::Identity);
    m_Effect->Apply(Renderer::GetDeviceContext());
}