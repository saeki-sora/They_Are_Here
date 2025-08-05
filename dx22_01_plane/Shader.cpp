#include "Shader.h"
#include "ShaderManager.h"
#include "renderer.h"

//=======================================
// Shader作成
//=======================================
void Shader::Create(std::string vs, std::string ps) 
{
    // ShaderManagerからシェーダーデータを取得（キャッシュから取得、なければ作成）
    m_shaderData = ShaderManager::GetInstance().GetShader(vs, ps);

    if (!m_shaderData) {
        MessageBox(nullptr, "Shader creation failed", "error", MB_OK);
    }
}

//=======================================
// GPUにデータを送る
//=======================================
void Shader::SetGPU()
{
    if (!m_shaderData) return;

    Renderer::BindVS(m_shaderData->vertexShader.Get());
    Renderer::BindPS(m_shaderData->pixelShader.Get());
    Renderer::BindIL(m_shaderData->inputLayout.Get());
}
