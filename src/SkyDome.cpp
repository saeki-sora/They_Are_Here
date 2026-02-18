#include "pch.h"
#include "SkyDome.h"
#include "Game.h"
#include"Player.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

SkyDome::SkyDome(const DirectX::SimpleMath::Vector3& pos, const DirectX::SimpleMath::Vector3& size)
    : VisualObject(pos, size)
{
}

void SkyDome::Init()
{
    // 球体の頂点とインデックスを生成
    std::vector<VERTEX_3D> vertices;
    std::vector<unsigned int> indices;

    const int sliceCount = 32;
    const int stackCount = 32;
    const float radius = 1.0f;

    for (int i = 0; i <= stackCount; ++i)
    {
        float phi = static_cast<float>(M_PI) * i / stackCount;
        for (int j = 0; j <= sliceCount; ++j)
        {
            float theta = 2.0f * static_cast<float>(M_PI) * j / sliceCount;

            VERTEX_3D vertex;
            vertex.position.x = radius * sinf(phi) * cosf(theta);
            vertex.position.y = radius * cosf(phi);
            vertex.position.z = radius * sinf(phi) * sinf(theta);
            vertex.normal = vertex.position;
            vertex.normal.Normalize();
            vertex.uv.x = (float)j / sliceCount;
            vertex.uv.y = (float)i / stackCount;
            vertex.color = DirectX::SimpleMath::Color(1, 1, 1, 1);
            vertices.push_back(vertex);
        }
    }

    // インデックス生成 
    for (int i = 0; i < stackCount; ++i)
    {
        for (int j = 0; j < sliceCount; ++j)
        {
            int first = (i * (sliceCount + 1)) + j;
            int second = first + sliceCount + 1;
            indices.push_back(first);
            indices.push_back(first + 1);
            indices.push_back(second);
            indices.push_back(second);
            indices.push_back(first + 1);
            indices.push_back(second + 1);
        }
    }

    m_indexCount = static_cast<UINT>(indices.size());
    m_VertexBuffer.Create(vertices);
    m_IndexBuffer.Create(indices);
    m_Shader.Create("shader/skyDomeVS.hlsl", "shader/skyDomePS.hlsl");

    m_Materiale = std::make_unique<Material>();
    MATERIAL mtrl;
    mtrl.Diffuse = DirectX::SimpleMath::Color(1, 1, 1, 1);
    mtrl.TextureEnable = true;
    m_Materiale->Create(mtrl);

    D3D11_RASTERIZER_DESC rasterDesc = {};
    rasterDesc.FillMode = D3D11_FILL_SOLID;
    rasterDesc.CullMode = D3D11_CULL_FRONT;
    Renderer::GetDevice()->CreateRasterizerState(&rasterDesc, m_rasterizerState.GetAddressOf());


    D3D11_DEPTH_STENCIL_DESC depthDesc = {};
    depthDesc.DepthEnable = true;
    depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    depthDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
    Renderer::GetDevice()->CreateDepthStencilState(&depthDesc, m_depthStencilState.GetAddressOf());
}

void SkyDome::Update(float deltaTime)
{

}

void SkyDome::Draw()
{

    DirectX::SimpleMath::Matrix s = DirectX::SimpleMath::Matrix::CreateScale(m_Scale);
    DirectX::SimpleMath::Matrix r = DirectX::SimpleMath::Matrix::CreateFromYawPitchRoll(m_Rotation.y, m_Rotation.x, m_Rotation.z);
    Matrix worldMtx = s * r;
    Renderer::SetWorldMatrix(&worldMtx);

    ID3D11DeviceContext* context = Renderer::GetDeviceContext();

    ID3D11RasterizerState* pOldRSState = nullptr;
    context->RSGetState(&pOldRSState);
    context->RSSetState(m_rasterizerState.Get());

    ID3D11DepthStencilState* pOldDSState = nullptr;
    UINT oldStencilRef;
    context->OMGetDepthStencilState(&pOldDSState, &oldStencilRef);
    context->OMSetDepthStencilState(m_depthStencilState.Get(), 1);

    // 描画処理
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_Shader.SetGPU();

    ID3D11InputLayout* il = nullptr;
    Renderer::GetDeviceContext()->IAGetInputLayout(&il);


    m_VertexBuffer.SetGPU();
    m_IndexBuffer.SetGPU();
    m_Texture.SetGPU();
    m_Materiale->SetGPU();

    context->DrawIndexed(m_indexCount, 0, 0);

    context->RSSetState(pOldRSState);
    if (pOldRSState) pOldRSState->Release();
    context->OMSetDepthStencilState(pOldDSState, oldStencilRef);
    if (pOldDSState) pOldDSState->Release();
}

bool SkyDome::Is3D() const
{
    // スカイドームは3Dオブジェクトとして描画する必要があるためtrueを返す
    return true;
}

