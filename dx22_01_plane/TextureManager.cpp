#include "TextureManager.h"
#include "Renderer.h" 
#include <d3d11.h>
#include <wrl/client.h>
#include <filesystem>
using std::filesystem::path;
using Microsoft::WRL::ComPtr;


static std::string NormalizeKey(const std::string& p) {
    if (p.empty()) return {};
    path abs = std::filesystem::weakly_canonical(path(p));
    std::string s = abs.generic_string();
    for (auto& c : s) c = (char)std::tolower((unsigned char)c);
    return s;
}


TextureManager::TextureManager()
{
    m_DefaultTexture = std::make_shared<Texture>();

    // device が未初期化のタイミングで呼ばれる可能性に備えてガード
    ID3D11Device* dev = Renderer::GetDevice();
    if (!dev) {
        m_DefaultTexture.reset(); // 後で Load() から再生成します
        return;
    }

    ComPtr<ID3D11ShaderResourceView> defaultSRV;
    if (CreateDefaultTexture(dev, defaultSRV.GetAddressOf())) {
        m_DefaultTexture->CreateFromSRV(defaultSRV);
    }
    else {
        m_DefaultTexture.reset();
    }
}

bool TextureManager::CreateDefaultTexture(ID3D11Device* device, ID3D11ShaderResourceView** srv) {
    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = 1; desc.Height = 1; desc.MipLevels = 1; desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    const uint32_t pixel = 0xFFFFFFFF;
    D3D11_SUBRESOURCE_DATA data{};
    data.pSysMem = &pixel;
    data.SysMemPitch = sizeof(uint32_t);

    ComPtr<ID3D11Texture2D> tex;
    if (FAILED(device->CreateTexture2D(&desc, &data, tex.GetAddressOf()))) return false;
    return SUCCEEDED(device->CreateShaderResourceView(tex.Get(), nullptr, srv));
}

std::shared_ptr<Texture> TextureManager::Load(const std::string& path) 
{
    // デフォルト未生成ならここで再生成を試みる
    if (!m_DefaultTexture) {
        if (auto* dev = Renderer::GetDevice()) {
            ComPtr<ID3D11ShaderResourceView> defaultSRV;
            if (CreateDefaultTexture(dev, defaultSRV.GetAddressOf())) {
                m_DefaultTexture = std::make_shared<Texture>();
                m_DefaultTexture->CreateFromSRV(defaultSRV);
            }
        }
    }

    if (path.empty()) {
        return m_DefaultTexture; // 空パスは既定を返す
    }

    const std::string key = NormalizeKey(path);
    if (auto it = m_Textures.find(key); it != m_Textures.end()) {
        return it->second;
    }

    auto tex = std::make_shared<Texture>();

    if (tex->Load(path)) {
        m_Textures[key] = tex;
        return tex;
    }
    return m_DefaultTexture;// 失敗時は既定でフォールバック
}
