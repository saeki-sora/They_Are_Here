#pragma once
#include <string>
#include <map>
#include <memory>
#include "Texture.h"

// ★ D3D 型は前方宣言だけ（ヘッダで <d3d11.h> を読まない）
struct ID3D11Device;
struct ID3D11ShaderResourceView;

class TextureManager {
private:
    std::map<std::string, std::shared_ptr<Texture>> m_Textures;
    std::shared_ptr<Texture> m_DefaultTexture;

    TextureManager();
    ~TextureManager() {}
    TextureManager(const TextureManager&) = delete;
    TextureManager& operator=(const TextureManager&) = delete;

    bool CreateDefaultTexture(ID3D11Device* device, ID3D11ShaderResourceView** srv);

public:
    static TextureManager& GetInstance() {
        static TextureManager instance;
        return instance;
    }

    std::shared_ptr<Texture> Load(const std::string& path);
};
