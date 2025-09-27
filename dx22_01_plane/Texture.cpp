#include <iostream>
#include "Texture.h"
#include "renderer.h"
#include <fstream>
#include <vector>
#include <filesystem>

// stb_image は .png, .jpg 用
#include "stb_image.h"

// DDSTextureLoader は .dds 用
#include "DDSTextureLoader.h"
#include <Windows.h> 

// 文字列をUTF-8としてワイド文字列に変換する、より堅牢なヘルパー関数
std::wstring StringToWString(const std::string& str)
{
    if (str.empty())
    {
        return std::wstring();
    }
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}


// テクスチャをロード
bool Texture::Load(const std::string& filename)
{
    std::string ext = filename.substr(filename.find_last_of('.'));
    ID3D11Device* device = Renderer::GetDevice();

    if (ext == ".dds" || ext == ".DDS")
    {
        // 実在チェック（デバッグに有用）
        if (!std::ifstream(filename, std::ios::binary).good()) {
            std::cout << "[DDS] File not found: " << filename
                << " (abs: " << std::filesystem::absolute(filename).string() << ")\n";
            return false;
        }

        // バイト列として読み込み → メモリから作成
        std::ifstream ifs(filename, std::ios::binary);
        std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(ifs)),
            std::istreambuf_iterator<char>());

        HRESULT hr = DirectX::CreateDDSTextureFromMemory(
            device,
            bytes.data(), (size_t)bytes.size(),
            nullptr, m_srv.GetAddressOf()
        );
        if (FAILED(hr)) {
            std::cout << "[DDS] Load error(mem): " << filename
                << " hr=0x" << std::hex << hr << std::dec << "\n";
            return false;
        }
        return true;
    }

    // --- PNG/JPG など（stb_image） ---
    int w = 0, h = 0, bpp = 0;
    unsigned char* pixels = stbi_load(filename.c_str(), &w, &h, &bpp, 4);
    if (!pixels) { std::cout << "[STB] Load error: " << filename << "\n"; return false; }
    m_width = w; m_height = h; m_bpp = 4;

    ComPtr<ID3D11Texture2D> tex;
    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = w; desc.Height = h; desc.MipLevels = 1; desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT; desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA srd{}; srd.pSysMem = pixels; srd.SysMemPitch = w * 4;
    HRESULT hr = device->CreateTexture2D(&desc, &srd, tex.GetAddressOf());
    if (FAILED(hr)) { stbi_image_free(pixels); return false; }

    hr = device->CreateShaderResourceView(tex.Get(), nullptr, m_srv.GetAddressOf());
    stbi_image_free(pixels);
    return SUCCEEDED(hr);
}


// ... (LoadFromFemory と SetGPU は変更なし) ...
bool Texture::LoadFromFemory(const unsigned char* Data, int len) {

    bool sts = true;
    unsigned char* pixels;

    // 画像読み込み
    pixels = stbi_load_from_memory(Data,
        len,
        &m_width,
        &m_height,
        &m_bpp,
        STBI_rgb_alpha);

    // テクスチャ2Dリソース生成
    ComPtr<ID3D11Texture2D> pTexture;

    D3D11_TEXTURE2D_DESC desc;
    ZeroMemory(&desc, sizeof(desc));

    desc.Width = m_width;
    desc.Height = m_height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;			// RGBA
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA subResource{};
    subResource.pSysMem = pixels;
    subResource.SysMemPitch = desc.Width * 4;			// RGBA = 4 bytes per pixel
    subResource.SysMemSlicePitch = 0;

    ID3D11Device* device = Renderer::GetDevice();

    HRESULT hr = device->CreateTexture2D(&desc, &subResource, pTexture.GetAddressOf());
    if (FAILED(hr)) {
        stbi_image_free(pixels);
        return false;
    }

    // SRV生成
    hr = device->CreateShaderResourceView(pTexture.Get(), nullptr, m_srv.GetAddressOf());
    if (FAILED(hr)) {
        stbi_image_free(pixels);
        return false;
    }

    // ピクセルイメージ解放
    stbi_image_free(pixels);

    return true;
}

// テクスチャをGPUにセット
void Texture::SetGPU()
{
    ID3D11DeviceContext* ctx = Renderer::GetDeviceContext();
    if (!ctx) return;

    ID3D11ShaderResourceView* srv = m_srv.Get(); // null 可（unbind）
    ID3D11ShaderResourceView * srvs[1] = { srv };
    ctx->PSSetShaderResources(0, 1, srvs);
}


void Texture::CreateFromSRV(ComPtr<ID3D11ShaderResourceView> srv)
{
    // 外部から渡されたSRVをメンバ変数にセット
    m_srv = srv;

    // デフォルトテクスチャ用の情報をセット
    m_texname = "DefaultTexture"; 
    m_width = 1;
    m_height = 1;
    m_bpp = 4;
}