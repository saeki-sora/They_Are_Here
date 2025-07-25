#include "ShaderManager.h"

ShaderManager::ShaderManager(){}
ShaderManager::~ShaderManager(){}
std::shared_ptr<Shader> ShaderManager::GetShader(std::string vs, std::string ps)
{
    std::string key = vs + ps;
    auto it = m_ShaderCache.find(key);
    if (it != m_ShaderCache.end()) {
        // 既にキャッシュに存在する場合、キャッシュから取得
        return it->second;
    }

    std::shared_ptr<Shader> newShader = std::make_shared<Shader>();
    newShader->Create(vs, ps);
    m_ShaderCache[key] = newShader;
    return newShader;
}