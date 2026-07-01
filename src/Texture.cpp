#include "pch.h"
#include "Texture.h"
#include "renderer.h"
#include "stb_image.h"
#include "DDSTextureLoader.h"

ComPtr<ID3D11ShaderResourceView> Texture::s_FlatNormalSrv{}; // 平坦ノーマルマップ（共有）

namespace
{
	// 自動生成ノーマルマップの凹凸の強さ
	constexpr float NORMAL_GEN_STRENGTH = 2.0f;

	// RGBA8ピクセル配列からミップマップ付きSRVを作成する
	bool CreateSRVFromPixels(const unsigned char* pixels, int width, int height,
		ComPtr<ID3D11ShaderResourceView>& outSrv)
	{
		ID3D11Device* device = Renderer::GetDevice();
		ID3D11DeviceContext* ctx = Renderer::GetDeviceContext();
		if (!device || !ctx) return false;

		ComPtr<ID3D11Texture2D> tex;
		D3D11_TEXTURE2D_DESC desc{};
		desc.Width = width;
		desc.Height = height;
		desc.MipLevels = 0;
		desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.SampleDesc.Count = 1;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
		desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

		HRESULT hr = device->CreateTexture2D(&desc, nullptr, tex.GetAddressOf());
		if (FAILED(hr)) return false;

		ctx->UpdateSubresource(tex.Get(), 0, nullptr, pixels, width * 4, 0);

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Format = desc.Format;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = -1;
		hr = device->CreateShaderResourceView(tex.Get(), &srvDesc, outSrv.GetAddressOf());
		if (FAILED(hr)) return false;

		ctx->GenerateMips(outSrv.Get());
		return true;
	}
}

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
bool Texture::Load(const std::string& filename, bool generateNormalMap)
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
#ifdef _DEBUG
            std::cout << "[DDS] File not found: " << filename << "\n";
#endif
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
#ifdef _DEBUG
            std::cout << "[DDS] Load error: " << filename << "\n";
#endif
            return false;
        }

        // ミップマップを生成実行
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
#ifdef _DEBUG
        std::cout << "[STB] Load error: " << filename << "\n";
#endif
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

    // ノーマルマップを用意（"_n"ファイル優先、無ければアルベドから自動生成）
    // 2D/UI（unlitシェーダー）は参照しないため、generateNormalMap=false でスキップ可能
    if (generateNormalMap && SUCCEEDED(hr) && !LoadNormalMapFile(filename))
    {
        GenerateNormalMap(pixels, w, h);
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
#ifdef _DEBUG
        std::cout << "[STB] Load from memory error\n";
#endif
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

    // 埋め込みテクスチャもアルベドからノーマルマップを自動生成する
    if (SUCCEEDED(hr))
    {
        GenerateNormalMap(pixels, m_width, m_height);
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

    // ノーマルマップをt2へ（未所持なら平坦ノーマルで凹凸なし）
    ID3D11ShaderResourceView* normalSrv = m_normalSrv ? m_normalSrv.Get() : GetFlatNormalSRV();
    ctx->PSSetShaderResources(2, 1, &normalSrv);
}

// "<名前>_n.<拡張子>" のノーマルマップ画像があればロードする
bool Texture::LoadNormalMapFile(const std::string& albedoFilename)
{
    size_t dotPos = albedoFilename.find_last_of('.');
    if (dotPos == std::string::npos) return false;

    std::string normalPath = albedoFilename.substr(0, dotPos) + "_n" + albedoFilename.substr(dotPos);
    if (!std::ifstream(normalPath, std::ios::binary).good()) return false;

    int w = 0, h = 0, bpp = 0;
    unsigned char* pixels = stbi_load(normalPath.c_str(), &w, &h, &bpp, 4);
    if (!pixels) return false;

    bool result = CreateSRVFromPixels(pixels, w, h, m_normalSrv);
    stbi_image_free(pixels);
    return result;
}

// アルベドのピクセルデータからSobelフィルタでノーマルマップを生成する
bool Texture::GenerateNormalMap(const unsigned char* pixels, int width, int height)
{
    if (!pixels || width <= 2 || height <= 2) return false;

    // 輝度マップを作成
    std::vector<float> luma(static_cast<size_t>(width) * height);
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            const unsigned char* p = &pixels[(static_cast<size_t>(y) * width + x) * 4];
            luma[static_cast<size_t>(y) * width + x] = (0.299f * p[0] + 0.587f * p[1] + 0.114f * p[2]) / 255.0f;
        }
    }

    // Sobelフィルタで勾配を求め、法線へ変換する（端はクランプ）
    std::vector<unsigned char> normalPixels(static_cast<size_t>(width) * height * 4);
    auto sampleLuma = [&](int x, int y)
    {
        x = std::clamp(x, 0, width - 1);
        y = std::clamp(y, 0, height - 1);
        return luma[static_cast<size_t>(y) * width + x];
    };

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            float dx = (sampleLuma(x + 1, y - 1) + 2.0f * sampleLuma(x + 1, y) + sampleLuma(x + 1, y + 1))
                     - (sampleLuma(x - 1, y - 1) + 2.0f * sampleLuma(x - 1, y) + sampleLuma(x - 1, y + 1));
            float dy = (sampleLuma(x - 1, y + 1) + 2.0f * sampleLuma(x, y + 1) + sampleLuma(x + 1, y + 1))
                     - (sampleLuma(x - 1, y - 1) + 2.0f * sampleLuma(x, y - 1) + sampleLuma(x + 1, y - 1));

            // 勾配の逆方向へ傾けた法線を正規化して0〜255へエンコードする
            DirectX::SimpleMath::Vector3 n(-dx * NORMAL_GEN_STRENGTH, -dy * NORMAL_GEN_STRENGTH, 1.0f);
            n.Normalize();

            unsigned char* dst = &normalPixels[(static_cast<size_t>(y) * width + x) * 4];
            dst[0] = static_cast<unsigned char>((n.x * 0.5f + 0.5f) * 255.0f);
            dst[1] = static_cast<unsigned char>((n.y * 0.5f + 0.5f) * 255.0f);
            dst[2] = static_cast<unsigned char>((n.z * 0.5f + 0.5f) * 255.0f);
            dst[3] = 255;
        }
    }

    return CreateSRVFromPixels(normalPixels.data(), width, height, m_normalSrv);
}

// 平坦ノーマルマップを取得する（未作成なら生成）
ID3D11ShaderResourceView* Texture::GetFlatNormalSRV()
{
    if (!s_FlatNormalSrv)
    {
        // 真上を向いた法線(0,0,1)のみの1x1テクスチャ
        const unsigned char flatPixel[4] = { 128, 128, 255, 255 };

        ID3D11Device* device = Renderer::GetDevice();
        if (!device) return nullptr;

        ComPtr<ID3D11Texture2D> tex;
        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = 1;
        desc.Height = 1;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA initData{};
        initData.pSysMem = flatPixel;
        initData.SysMemPitch = 4;

        if (FAILED(device->CreateTexture2D(&desc, &initData, tex.GetAddressOf()))) return nullptr;
        if (FAILED(device->CreateShaderResourceView(tex.Get(), nullptr, s_FlatNormalSrv.GetAddressOf()))) return nullptr;
    }
    return s_FlatNormalSrv.Get();
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
#ifdef _DEBUG
        std::cout << "Texture::SetWidth: invalid width (" << width << "), clamping to 1\n";
#endif
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
#ifdef _DEBUG
        std::cout << "Texture::SetHeight: invalid height (" << height << "), clamping to 1\n";
#endif
        m_height = 1;
    }
    else
    {
        m_height = height;
    }
}