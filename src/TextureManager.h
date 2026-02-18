#pragma once

#include "Texture.h"

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
