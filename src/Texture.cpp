#include "pch.h"
#include "Texture.h"
#include "renderer.h"
#include "stb_image.h"
#include "DDSTextureLoader.h"

// 文字列をUTF-8としてワイド文字列に変換する
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
    ID3D11DeviceContext* ctx = Renderer::GetDeviceContext();

    // ==========================================================
    // DDSファイルの読み込み処理
    // ==========================================================
    if (ext == ".dds" || ext == ".DDS")
    {
        if (!std::ifstream(filename, std::ios::binary).good())
        {
            std::cout << "[DDS] File not found: " << filename << "\n";
            return false;
        }

        std::ifstream ifs(filename, std::ios::binary);
        std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(ifs)),
            std::istreambuf_iterator<char>());

        
        HRESULT hr = DirectX::CreateDDSTextureFromMemoryEx(
            device,
            bytes.data(),
            bytes.size(),
            0,
            D3D11_USAGE_DEFAULT,
            D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET,
            0,
            D3D11_RESOURCE_MISC_GENERATE_MIPS,
            false,
            nullptr,
            m_srv.GetAddressOf()
        );

        if (FAILED(hr))
        {
            // Exが失敗した場合、通常の読み込みを試す
            hr = DirectX::CreateDDSTextureFromMemory(
                device,
                bytes.data(), (size_t)bytes.size(),
                nullptr, m_srv.GetAddressOf()
            );
        }

        if (FAILED(hr))
        {
            std::cout << "[DDS] Load error: " << filename << "\n";
            return false;
        }

        // ★ミップマップを生成実行
        if (ctx)
        {
            ctx->GenerateMips(m_srv.Get());
        }

        return true;
    }

    // ==========================================================
    // 通常画像の読み込み処理 (stb_image)
    // ==========================================================
    int w = 0, h = 0, bpp = 0;
    unsigned char* pixels = stbi_load(filename.c_str(), &w, &h, &bpp, 4);
    if (!pixels)
    {
        std::cout << "[STB] Load error: " << filename << "\n";
        return false;
    }
    m_width = w;
    m_height = h;
    m_bpp = 4;

    ComPtr<ID3D11Texture2D> tex;
    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = w;
    desc.Height = h;
    desc.MipLevels = 0; // 全レベル作成
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS; // ★生成フラグ

    HRESULT hr = device->CreateTexture2D(&desc, nullptr, tex.GetAddressOf());
    if (FAILED(hr)) { stbi_image_free(pixels); return false; }

    // Level 0 (最大サイズ) を転送
    if (ctx)
    {
        ctx->UpdateSubresource(tex.Get(), 0, nullptr, pixels, w * 4, 0);
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = desc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = -1; // 全ミップを使用
    hr = device->CreateShaderResourceView(tex.Get(), &srvDesc, m_srv.GetAddressOf());

    // ★ミップマップ生成実行
    if (SUCCEEDED(hr) && ctx)
    {
        ctx->GenerateMips(m_srv.Get());
    }

    stbi_image_free(pixels);
    return SUCCEEDED(hr);
}


bool Texture::LoadFromMemory(const unsigned char* Data, int len) {

    unsigned char* pixels;

    // 画像読み込み
    pixels = stbi_load_from_memory(Data,
        len,
        &m_width,
        &m_height,
        &m_bpp,
        STBI_rgb_alpha);

    if (!pixels) {
        std::cout << "[STB] Load from memory error\n";
        return false;
    }

    ID3D11Device* device = Renderer::GetDevice();
    ID3D11DeviceContext* ctx = Renderer::GetDeviceContext();

    ComPtr<ID3D11Texture2D> pTexture;
    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = m_width;
    desc.Height = m_height;
    desc.MipLevels = 0; // ミップチェーン全体を確保
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET; // レンダーターゲットとしても使えるように
    desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

    // まずはデータなしでテクスチャリソースを作成
    HRESULT hr = device->CreateTexture2D(&desc, nullptr, pTexture.GetAddressOf());
    if (FAILED(hr)) {
        stbi_image_free(pixels);
        return false;
    }

    // UpdateSubresourceでピクセルデータを転送
    ctx->UpdateSubresource(pTexture.Get(), 0, nullptr, pixels, m_width * 4, 0);

    // SRV(シェーダーリソースビュー)の詳細設定を作成
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = desc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = -1; // -1は全てのミップレベルをSRVに含めるという意味

    // 詳細設定を使ってSRVを作成
    hr = device->CreateShaderResourceView(pTexture.Get(), &srvDesc, m_srv.GetAddressOf());

    // ミップマップ自動生成
    if (SUCCEEDED(hr))
    {
        ctx->GenerateMips(m_srv.Get());
    }

    // ピクセルイメージ解放
    stbi_image_free(pixels);

    return SUCCEEDED(hr);
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


void Texture::SetWidth(int width)
{
    if (width <= 0)
    {
        std::cout << "Texture::SetWidth: invalid width (" << width << "), clamping to 1\n";
        m_width = 1;
    }
    else
    {
        m_width = width;
    }
}

void Texture::SetHeight(int height)
{
    if (height <= 0)
    {
        std::cout << "Texture::SetHeight: invalid height (" << height << "), clamping to 1\n";
        m_height = 1;
    }
    else
    {
        m_height = height;
    }
}