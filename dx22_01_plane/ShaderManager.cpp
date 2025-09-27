#include "ShaderManager.h"
#include "dx11helper.h"
#include "renderer.h"
#include <iostream>

//-----------------------------------------------------------------------------
// シングルトンインスタンス取得
//-----------------------------------------------------------------------------
ShaderManager& ShaderManager::GetInstance() 
{
    static ShaderManager instance;
    return instance;
}

//-----------------------------------------------------------------------------
// シェーダー取得（キャッシュから取得、なければ作成）
//-----------------------------------------------------------------------------
std::shared_ptr<ShaderData> ShaderManager::GetShader(const std::string& vsPath, const std::string& psPath)
{
    // キャッシュキーを生成
    std::string cacheKey = GenerateCacheKey(vsPath, psPath);

    // キャッシュから検索
    auto it = m_shaderCache.find(cacheKey);
    if (it != m_shaderCache.end()) {
        // キャッシュヒット：既存のシェーダーを返す
        return it->second;
    }

    // キャッシュミス：新しいシェーダーを作成
    auto shaderData = CreateShader(vsPath, psPath);
    if (shaderData) {
        // キャッシュに保存
        m_shaderCache[cacheKey] = shaderData;
    }

    return shaderData;
}

//-----------------------------------------------------------------------------
// シェーダーを実際に作成する
//-----------------------------------------------------------------------------
std::shared_ptr<ShaderData> ShaderManager::CreateShader(const std::string& vsPath, const std::string& psPath)
{
    auto shaderData = std::make_shared<ShaderData>();

    // 頂点データの定義
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };
    unsigned int numElements = ARRAYSIZE(layout);

    ID3D11Device* device = Renderer::GetDevice();

    // 頂点シェーダーオブジェクトを生成
    bool sts = CreateVertexShader(
        device,
        vsPath.c_str(),
        "vs_main",
        "vs_5_0",
        layout,
        numElements,
        &shaderData->vertexShader,
        &shaderData->inputLayout
    );

    if (!sts) {
        MessageBox(nullptr, ("CreateVertexShader error: " + vsPath).c_str(), "error", MB_OK);
        return nullptr;
    }

    // ピクセルシェーダーを生成
    sts = CreatePixelShader(
        device,
        psPath.c_str(),
        "ps_main",
        "ps_5_0",
        &shaderData->pixelShader
    );

    if (!sts) {
        MessageBox(nullptr, ("CreatePixelShader error: " + psPath).c_str(), "error", MB_OK);
        return nullptr;
    }

    return shaderData;
}

//-----------------------------------------------------------------------------
// キャッシュキーを生成
//-----------------------------------------------------------------------------
std::string ShaderManager::GenerateCacheKey(const std::string& vsPath, const std::string& psPath) {
    return vsPath + "|" + psPath;
}

//-----------------------------------------------------------------------------
// キャッシュをクリア
//-----------------------------------------------------------------------------
void ShaderManager::ClearCache() {
    m_shaderCache.clear();
}