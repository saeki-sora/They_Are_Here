#include "pch.h"
#include "Renderer.h"
#include "SkinnedModel.h" 
#include "Application.h"
#include"ImGUI_Manager.h"
#include"Shader.h"
#include "SkinnedMesh.h"
#include "PostProcessManager.h"

using namespace DirectX::SimpleMath;

// D3D_FEATURE_LEVELはDirect3Dのバージョン
D3D_FEATURE_LEVEL Renderer::m_FeatureLevel = D3D_FEATURE_LEVEL_11_0;

ID3D11Device* Renderer::m_Device{}; // Direct3Dデバイス
ID3D11DeviceContext* Renderer::m_DeviceContext{}; // デバイスコンテキスト
IDXGISwapChain* Renderer::m_SwapChain{}; // スワップチェーン
ID3D11RenderTargetView* Renderer::m_RenderTargetView{}; // レンダーターゲットビュー
ID3D11RenderTargetView* Renderer::m_RenderTargetView_Mirror{}; //鏡用のレンダーターゲット
ID3D11DepthStencilView* Renderer::m_DepthStencilView{}; // デプスステンシルビュー
ID3D11ShaderResourceView* Renderer::m_DepthSRV{}; // 深度バッファのSRV（ポストプロセス用）

ID3D11Buffer* Renderer::m_WorldBuffer{}; // ワールド行列
ID3D11Buffer* Renderer::m_WorldInverseTransposeBuffer{}; // ワールド逆転置行列
ID3D11Buffer* Renderer::m_ViewBuffer{}; // ビュー行列
ID3D11Buffer* Renderer::m_ProjectionBuffer{}; // プロジェクション行列

ID3D11Buffer* Renderer::m_LightBuffer{};//ライト設定(平行光源)
ID3D11Buffer* Renderer::m_MaterialBuffer{};//マテリアル設定
ID3D11Buffer* Renderer::m_TextureBuffer{};//マテリアル設定

ID3D11Buffer* Renderer::m_SkinningBuffer = nullptr;

ID3D11Buffer* Renderer::m_UVAdjustBuffer = nullptr;

ID3D11SamplerState* Renderer::m_SamplerState = nullptr;

// デプスステンシルステート
ID3D11DepthStencilState* Renderer::m_DepthStateEnable{};
ID3D11DepthStencilState* Renderer::m_DepthStateDisable{};

ID3D11BlendState* Renderer::m_BlendState[MAX_BLENDSTATE]; // ブレンドステート配列
ID3D11BlendState* Renderer::m_BlendStateATC{}; // 特定のアルファテストとカバレッジ（ATC）用のブレンドステート

DirectX::SimpleMath::Matrix Renderer::m_ViewMatrix = DirectX::SimpleMath::Matrix::Identity;
DirectX::SimpleMath::Matrix Renderer::m_ProjectionMatrix = DirectX::SimpleMath::Matrix::Identity;
DirectX::SimpleMath::Matrix Renderer::m_WorldMatrix = DirectX::SimpleMath::Matrix::Identity;

Shader Renderer::m_SkinnedShader{};

LIGHT_CONSTANT_BUFFER Renderer::m_LightData{};//現在のライト情報を保持する変数

// シャドウマップ用リソース
ID3D11Texture2D*          Renderer::m_ShadowMapTex   = nullptr;
ID3D11DepthStencilView*   Renderer::m_ShadowDSV      = nullptr;
ID3D11ShaderResourceView* Renderer::m_ShadowSRV      = nullptr;
ID3D11SamplerState*       Renderer::m_ShadowSampler  = nullptr;
ID3D11Buffer*             Renderer::m_ShadowBuffer   = nullptr;
ID3D11RasterizerState*    Renderer::m_ShadowRS       = nullptr;
ID3D11RasterizerState*    Renderer::m_NormalRS       = nullptr;
Shader                    Renderer::m_ShadowStaticShader{};
Shader                    Renderer::m_ShadowSkinnedShader{};

namespace 
{
	struct StateCache {
		ID3D11VertexShader* VS = nullptr;
		ID3D11PixelShader* PS = nullptr;
		ID3D11InputLayout* IL = nullptr;
	} g_state;
}

// ImGUI_Manager のインスタンス
//ImGUI_Manager g_ImGuiManager;

// 最高性能のアダプターを選んで返す
static IDXGIAdapter1* PickBestAdapter()
{
    // DXGI 1.6 で HIGH_PERFORMANCE を要求（Optimus/MXM も正しく扱う）
    IDXGIFactory6* factory6 = nullptr;
    if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory6))))
    {
        IDXGIAdapter1* adapter = nullptr;
        HRESULT hr = factory6->EnumAdapterByGpuPreference(
            0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter));
        factory6->Release();
        if (SUCCEEDED(hr)) return adapter;
    }

    // 専用VRAMが最大のアダプターを選ぶ
    IDXGIFactory1* factory1 = nullptr;
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory1)))) return nullptr;

    IDXGIAdapter1* bestAdapter = nullptr;
    SIZE_T bestVram = 0;
    IDXGIAdapter1* adapter = nullptr;
    for (UINT i = 0; factory1->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i)
    {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) { adapter->Release(); continue; }
        if (desc.DedicatedVideoMemory > bestVram)
        {
            if (bestAdapter) bestAdapter->Release();
            bestAdapter = adapter;
            bestVram = desc.DedicatedVideoMemory;
        }
        else { adapter->Release(); }
    }
    factory1->Release();
    return bestAdapter;
}

//=======================================
//初期化処理
//=======================================
void Renderer::Init()
{
    HRESULT hr = S_OK;

	// ウインドウサイズ取得
    const UINT width = Application::GetWidth();
    const UINT height = Application::GetHeight();

    // ------------------------------------
    // デバイス & スワップチェーン作成
    // ------------------------------------
    DXGI_SWAP_CHAIN_DESC swapChainDesc{};
    swapChainDesc.BufferCount = 1;
    swapChainDesc.BufferDesc.Width = width;
    swapChainDesc.BufferDesc.Height = height;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow = Application::GetWindow();
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.Windowed = TRUE;

    // 最高性能GPUを選択（Optimus等でも外付けGPUを優先する）
    IDXGIAdapter1* bestAdapter = PickBestAdapter();
    hr = D3D11CreateDeviceAndSwapChain(
        bestAdapter,
        bestAdapter ? D3D_DRIVER_TYPE_UNKNOWN : D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,
        nullptr,
        0,
        D3D11_SDK_VERSION,
        &swapChainDesc,
        &m_SwapChain,
        &m_Device,
        &m_FeatureLevel,
        &m_DeviceContext);
    if (bestAdapter) bestAdapter->Release();

    if (FAILED(hr)) return;

    // Alt+Enterによる排他フルスクリーン切替を無効化（ボーダーレスフルスクリーン運用のため）
    IDXGIFactory* dxgiFactory = nullptr;
    if (SUCCEEDED(m_SwapChain->GetParent(__uuidof(IDXGIFactory), reinterpret_cast<void**>(&dxgiFactory))))
    {
        dxgiFactory->MakeWindowAssociation(Application::GetWindow(), DXGI_MWA_NO_ALT_ENTER);
        dxgiFactory->Release();
    }

    // ------------------------------------
    // バックバッファ → RTV を 2 個作成
    // ------------------------------------
    ID3D11Texture2D* backBuffer = nullptr;
    hr = m_SwapChain->GetBuffer(
        0,
        __uuidof(ID3D11Texture2D),
        reinterpret_cast<void**>(&backBuffer));
    if (FAILED(hr) || !backBuffer) return;

    hr = m_Device->CreateRenderTargetView(
        backBuffer, nullptr, &m_RenderTargetView);
    if (FAILED(hr)) { backBuffer->Release(); return; }

    hr = m_Device->CreateRenderTargetView(
        backBuffer, nullptr, &m_RenderTargetView_Mirror);
    if (FAILED(hr)) { backBuffer->Release(); return; }

    backBuffer->Release();

    // ------------------------------------
    // 深度ステンシルバッファ & DSV
    // ------------------------------------
    ID3D11Texture2D* depthStencilTex = nullptr;

    // ポストプロセスで深度を読めるよう TYPELESS で作成し、DSVとSRVを両方作る
    D3D11_TEXTURE2D_DESC depthDesc{};
    depthDesc.Width = width;
    depthDesc.Height = height;
    depthDesc.MipLevels = 1;
    depthDesc.ArraySize = 1;
    depthDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
    depthDesc.SampleDesc = swapChainDesc.SampleDesc;
    depthDesc.Usage = D3D11_USAGE_DEFAULT;
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    depthDesc.CPUAccessFlags = 0;
    depthDesc.MiscFlags = 0;

    hr = m_Device->CreateTexture2D(
        &depthDesc, nullptr, &depthStencilTex);
    if (FAILED(hr) || !depthStencilTex) return;

    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
    dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Flags = 0;

    hr = m_Device->CreateDepthStencilView(
        depthStencilTex, &dsvDesc, &m_DepthStencilView);
    if (FAILED(hr)) { depthStencilTex->Release(); return; }

    // 深度SRV（24bit深度部分のみ読む）
    D3D11_SHADER_RESOURCE_VIEW_DESC depthSrvDesc{};
    depthSrvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    depthSrvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    depthSrvDesc.Texture2D.MipLevels = 1;

    hr = m_Device->CreateShaderResourceView(
        depthStencilTex, &depthSrvDesc, &m_DepthSRV);
    depthStencilTex->Release();
    if (FAILED(hr)) return;

    m_DeviceContext->OMSetRenderTargets(
        1, &m_RenderTargetView, m_DepthStencilView);

    // ------------------------------------
    // ビューポート設定
    // ------------------------------------
    D3D11_VIEWPORT viewport{};
    viewport.Width = static_cast<FLOAT>(width);
    viewport.Height = static_cast<FLOAT>(height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    m_DeviceContext->RSSetViewports(1, &viewport);

    // ------------------------------------
    // ラスタライザステート
    // ------------------------------------
    D3D11_RASTERIZER_DESC rasterizerDesc{};
    rasterizerDesc.FillMode = D3D11_FILL_SOLID;
    rasterizerDesc.CullMode = D3D11_CULL_NONE;
    rasterizerDesc.DepthClipEnable = TRUE;

    ID3D11RasterizerState* rs = nullptr;
    hr = m_Device->CreateRasterizerState(&rasterizerDesc, &rs);
    if (FAILED(hr)) return;
    m_DeviceContext->RSSetState(rs);
    rs->Release();

    // ------------------------------------
    // ブレンドステート
    // ------------------------------------
    D3D11_BLEND_DESC blendDesc{};
    blendDesc.AlphaToCoverageEnable = FALSE;
    blendDesc.IndependentBlendEnable = TRUE;

    auto& rt0 = blendDesc.RenderTarget[0];
    rt0.SrcBlend = D3D11_BLEND_SRC_ALPHA;
    rt0.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    rt0.BlendOp = D3D11_BLEND_OP_ADD;
    rt0.SrcBlendAlpha = D3D11_BLEND_ONE;
    rt0.DestBlendAlpha = D3D11_BLEND_ZERO;
    rt0.BlendOpAlpha = D3D11_BLEND_OP_ADD;
    rt0.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    // 不透明（ブレンド無効）
    rt0.BlendEnable = FALSE;
    hr = m_Device->CreateBlendState(&blendDesc, &m_BlendState[BS_NONE]);
    if (FAILED(hr)) return;

    // アルファブレンド
    rt0.BlendEnable = TRUE;
    hr = m_Device->CreateBlendState(&blendDesc, &m_BlendState[BS_ALPHABLEND]);
    if (FAILED(hr)) return;

    // 加算
    rt0.DestBlend = D3D11_BLEND_ONE;
    hr = m_Device->CreateBlendState(&blendDesc, &m_BlendState[BS_ADDITIVE]);
    if (FAILED(hr)) return;

    // 減算
    rt0.BlendOp = D3D11_BLEND_OP_REV_SUBTRACT;
    hr = m_Device->CreateBlendState(&blendDesc, &m_BlendState[BS_SUBTRACTION]);
    if (FAILED(hr)) return;

    SetBlendState(BS_OPAQUE);

    // ------------------------------------
    // デプスステンシルステート
    // ------------------------------------
    D3D11_DEPTH_STENCIL_DESC depthStencilDesc{};
    depthStencilDesc.DepthEnable = TRUE;
    depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
    depthStencilDesc.StencilEnable = FALSE;

    hr = m_Device->CreateDepthStencilState(
        &depthStencilDesc, &m_DepthStateEnable);
    if (FAILED(hr)) return;

    depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    depthStencilDesc.DepthEnable = FALSE;
    hr = m_Device->CreateDepthStencilState(
        &depthStencilDesc, &m_DepthStateDisable);
    if (FAILED(hr)) return;

    m_DeviceContext->OMSetDepthStencilState(m_DepthStateEnable, 0);

    // ------------------------------------
    // サンプラーステート
    // ------------------------------------
    D3D11_SAMPLER_DESC samplerDesc{};
    samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.MipLODBias = 0.0f;
    samplerDesc.MaxAnisotropy = 16;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    samplerDesc.MinLOD = 0.0f;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    hr = m_Device->CreateSamplerState(&samplerDesc, &m_SamplerState);
    if (FAILED(hr)) return;
    m_DeviceContext->PSSetSamplers(0, 1, &m_SamplerState);

    // ------------------------------------
    // 各種定数バッファ
    // ------------------------------------
    D3D11_BUFFER_DESC cbDesc{};
    cbDesc.Usage = D3D11_USAGE_DEFAULT;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = 0;
    cbDesc.MiscFlags = 0;
    cbDesc.StructureByteStride = sizeof(float);

    // World
    cbDesc.ByteWidth = sizeof(Matrix);
    hr = m_Device->CreateBuffer(&cbDesc, nullptr, &m_WorldBuffer);
    if (FAILED(hr)) return;
    m_DeviceContext->VSSetConstantBuffers(0, 1, &m_WorldBuffer);

	// World Inverse Transpose
    cbDesc.ByteWidth = sizeof(Matrix);
    hr = m_Device->CreateBuffer(&cbDesc, nullptr, &m_WorldInverseTransposeBuffer);
    if (FAILED(hr)) return;
    // VSの b7 にバインド
    m_DeviceContext->VSSetConstantBuffers(7, 1, &m_WorldInverseTransposeBuffer);

    // Skinning
    D3D11_BUFFER_DESC bd{};
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.CPUAccessFlags = 0;
    bd.MiscFlags = 0;
    bd.ByteWidth = sizeof(Matrix) * 64;

    hr = m_Device->CreateBuffer(&bd, nullptr, &m_SkinningBuffer);
    if (FAILED(hr)) { assert(false && "Failed to create SkinningBuffer"); return; }

    // View
    cbDesc.ByteWidth = sizeof(Matrix);
    hr = m_Device->CreateBuffer(&cbDesc, nullptr, &m_ViewBuffer);
    if (FAILED(hr)) { assert(false && "Failed to create ViewBuffer"); return; }
    m_DeviceContext->VSSetConstantBuffers(1, 1, &m_ViewBuffer);

    // Projection
    hr = m_Device->CreateBuffer(&cbDesc, nullptr, &m_ProjectionBuffer);
    if (FAILED(hr)) { assert(false && "Failed to create ProjectionBuffer"); return; }
    m_DeviceContext->VSSetConstantBuffers(2, 1, &m_ProjectionBuffer);

    // Light
    cbDesc.ByteWidth = sizeof(LIGHT_CONSTANT_BUFFER);
    hr = m_Device->CreateBuffer(&cbDesc, nullptr, &m_LightBuffer);
    if (FAILED(hr)) return;
    m_DeviceContext->VSSetConstantBuffers(3, 1, &m_LightBuffer);
    m_DeviceContext->PSSetConstantBuffers(3, 1, &m_LightBuffer);

    // スキニング用シェーダ
    m_SkinnedShader.Create(
        "shader/litSkinnedVS.hlsl",
        "shader/litTexturePS.hlsl");

    // ライト初期化
    ClearLights();
    LIGHT light{};
    light.Enable = TRUE;
    light.Direction = Vector4(0.5f, -1.0f, 0.8f, 0.0f);
    light.Direction.Normalize();
    light.Diffuse = Color(0.55f, 0.62f, 0.85f, 1.0f);
    light.Ambient = Color(0.55f, 0.36f, 0.59f, 1.0f);
    m_LightData.RenderParams = Vector4(1.0f, 0.0f, 0.0f, 0.0f); // ノーマルマップ強度のデフォルト
    SetLight(light);

    // Material
    cbDesc.ByteWidth = sizeof(MATERIAL);
    hr = m_Device->CreateBuffer(&cbDesc, nullptr, &m_MaterialBuffer);
    if (FAILED(hr)) return;
    m_DeviceContext->PSSetConstantBuffers(4, 1, &m_MaterialBuffer);
    m_DeviceContext->VSSetConstantBuffers(4, 1, &m_MaterialBuffer);

    MATERIAL material{};
    material.Ambient = Color(1.0f, 1.0f, 1.0f, 1.0f);
    material.Diffuse = Color(1.0f, 1.0f, 1.0f, 1.0f);
    SetMaterial(material);

    // Texture transform
    cbDesc.ByteWidth = sizeof(Matrix);
    hr = m_Device->CreateBuffer(&cbDesc, nullptr, &m_TextureBuffer);
    if (FAILED(hr)) return;
    m_DeviceContext->VSSetConstantBuffers(5, 1, &m_TextureBuffer);

    SetUV(0, 0, 1, 1);

    // シャドウマップ用テクスチャ作成
    {
        D3D11_TEXTURE2D_DESC sdesc{};
        sdesc.Width              = SHADOW_MAP_SIZE;
        sdesc.Height             = SHADOW_MAP_SIZE;
        sdesc.MipLevels          = 1;
        sdesc.ArraySize          = 1;
        sdesc.Format             = DXGI_FORMAT_R32_TYPELESS;
        sdesc.SampleDesc.Count   = 1;
        sdesc.SampleDesc.Quality = 0;
        sdesc.Usage              = D3D11_USAGE_DEFAULT;
        sdesc.BindFlags          = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

        hr = m_Device->CreateTexture2D(&sdesc, nullptr, &m_ShadowMapTex);
        assert(SUCCEEDED(hr) && "ShadowMap Texture2D 作成失敗");

        // 深度ステンシルビュー作成
        D3D11_DEPTH_STENCIL_VIEW_DESC dsvd{};
        dsvd.Format        = DXGI_FORMAT_D32_FLOAT;
        dsvd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        hr = m_Device->CreateDepthStencilView(m_ShadowMapTex, &dsvd, &m_ShadowDSV);
        assert(SUCCEEDED(hr) && "ShadowMap DSV 作成失敗");

        // シェーダーリソースビュー作成
        D3D11_SHADER_RESOURCE_VIEW_DESC srvd{};
        srvd.Format              = DXGI_FORMAT_R32_FLOAT;
        srvd.ViewDimension       = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvd.Texture2D.MipLevels = 1;
        hr = m_Device->CreateShaderResourceView(m_ShadowMapTex, &srvd, &m_ShadowSRV);
        assert(SUCCEEDED(hr) && "ShadowMap SRV 作成失敗");
    }

    // シャドウ比較サンプラー作成
    {
        D3D11_SAMPLER_DESC sd{};
        sd.Filter         = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
        sd.AddressU       = D3D11_TEXTURE_ADDRESS_BORDER;
        sd.AddressV       = D3D11_TEXTURE_ADDRESS_BORDER;
        sd.AddressW       = D3D11_TEXTURE_ADDRESS_BORDER;
        sd.BorderColor[0] = 1.0f;  // 範囲外は影なし
        sd.BorderColor[1] = 1.0f;
        sd.BorderColor[2] = 1.0f;
        sd.BorderColor[3] = 1.0f;
        sd.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
        sd.MinLOD         = 0.0f;
        sd.MaxLOD         = D3D11_FLOAT32_MAX;
        hr = m_Device->CreateSamplerState(&sd, &m_ShadowSampler);
        assert(SUCCEEDED(hr) && "Shadow Sampler 作成失敗");
    }

    // シャドウ用定数バッファ作成 (b8: LightViewProj)
    {
        D3D11_BUFFER_DESC bd{};
        bd.ByteWidth      = sizeof(Matrix);
        bd.Usage          = D3D11_USAGE_DEFAULT;
        bd.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
        hr = m_Device->CreateBuffer(&bd, nullptr, &m_ShadowBuffer);
        assert(SUCCEEDED(hr) && "Shadow CBuffer 作成失敗");
    }

    // シャドウ用ラスタライザ作成 (深度バイアスあり)
    {
        D3D11_RASTERIZER_DESC rd{};
        rd.FillMode              = D3D11_FILL_SOLID;
        rd.CullMode              = D3D11_CULL_BACK;
        rd.DepthClipEnable       = TRUE;
        rd.DepthBias             = 1000;
        rd.DepthBiasClamp        = 0.0f;
        rd.SlopeScaledDepthBias  = 0.5f;
        hr = m_Device->CreateRasterizerState(&rd, &m_ShadowRS);
        assert(SUCCEEDED(hr) && "Shadow RS 作成失敗");
    }

    // 通常ラスタライザを Init 時に一度だけ生成（EndShadowPass で使い回す）
    {
        D3D11_RASTERIZER_DESC rd{};
        rd.FillMode        = D3D11_FILL_SOLID;
        rd.CullMode        = D3D11_CULL_NONE;
        rd.DepthClipEnable = TRUE;
        hr = m_Device->CreateRasterizerState(&rd, &m_NormalRS);
        assert(SUCCEEDED(hr) && "Normal RS 作成失敗");
    }

    // シャドウ用シェーダー作成
    m_ShadowStaticShader.Create("shader/shadowVS.hlsl", "shader/shadowPS.hlsl");
    m_ShadowSkinnedShader.Create("shader/shadowSkinnedVS.hlsl", "shader/shadowPS.hlsl");

    ImGUI_Manager::Init(Application::GetWindow(), m_Device, m_DeviceContext);
}




void Renderer::SetSkinningMatrices(
	const std::vector<Matrix>& boneMatrices)
{
	if (!m_DeviceContext || !m_SkinningBuffer)
		return;

	// HLSL 側と合わせた最大数
	constexpr std::size_t MAX_BONES = 64;

	// 一時バッファ
	Matrix bufferData[MAX_BONES];

	std::size_t count =
		std::min<std::size_t>(boneMatrices.size(), MAX_BONES);

	// 0〜count-1 をコピー
	for (std::size_t i = 0; i < count; ++i)
	{
		 bufferData[i] = boneMatrices[i].Transpose();
	}

	// 余っている分は単位行列で埋める
	for (std::size_t i = count; i < MAX_BONES; ++i)
	{
		bufferData[i] = Matrix::Identity;
	}

	// 定数バッファに書き込む
	m_DeviceContext->UpdateSubresource(
		m_SkinningBuffer,
		0,
		nullptr,
		bufferData,
		0,
		0);

	// VS の b6 にバインド
	ID3D11Buffer* cb = m_SkinningBuffer;
	m_DeviceContext->VSSetConstantBuffers(6, 1, &cb);
}




void Renderer::Draw(const SkinnedModel& model, const Matrix& world)
{
	const auto& mesh = model.GetMesh();
	const auto& bones = model.GetFinalMatrices();

	//シェーダー設定
	m_SkinnedShader.SetGPU();

	//ボーン行列の設定
	SetSkinningMatrices(bones);

	// World 行列
	Matrix w = world.Transpose();
	m_DeviceContext->UpdateSubresource(
		m_WorldBuffer, 0, nullptr, &w, 0, 0);

	// 頂点／インデックスバッファ設定
	UINT stride = sizeof(VERTEX_SKINNED);
	UINT offset = 0;
	m_DeviceContext->IASetVertexBuffers(0, 1, mesh.GetVertexBuffer(), &stride, &offset);
	m_DeviceContext->IASetIndexBuffer(mesh.GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);

	// サブセットごとに描画
	for (auto& sub : mesh.GetSubsets())
	{
		auto tex = mesh.GetTextures()[sub.MaterialIdx];
		tex->SetGPU(); // アルベド(t0)とノーマルマップ(t2)をバインド

		m_DeviceContext->DrawIndexed(sub.IndexNum, sub.IndexBase, 0);
	}
}





//=======================================
//描画状態の設定
//=======================================
void Renderer::BindVS(ID3D11VertexShader* vs)
{
	if (g_state.VS != vs) {
		m_DeviceContext->VSSetShader(vs, nullptr, 0);
		g_state.VS = vs;
	}
}

void Renderer::BindPS(ID3D11PixelShader* ps) 
{
	if (g_state.PS != ps) {
		m_DeviceContext->PSSetShader(ps, nullptr, 0);
		g_state.PS = ps;
	}
}

void Renderer::BindIL(ID3D11InputLayout* il)
{
	if (g_state.IL != il) {
		m_DeviceContext->IASetInputLayout(il);
		g_state.IL = il;
	}
}

void Renderer::ResetStateCache()
{
	g_state = {};
}

//=======================================
//終了処理
//=======================================
void Renderer::Uninit()
{
    m_WorldBuffer->Release();
    if (m_WorldInverseTransposeBuffer) m_WorldInverseTransposeBuffer->Release();
	m_ViewBuffer->Release();
	m_ProjectionBuffer->Release();

	m_LightBuffer->Release();
	m_MaterialBuffer->Release();

	m_DeviceContext->ClearState();
	if (m_DepthSRV) { m_DepthSRV->Release(); m_DepthSRV = nullptr; }
	m_RenderTargetView->Release();
	m_SwapChain->Release();
	m_DeviceContext->Release();
	m_Device->Release();
	if (m_SkinningBuffer) 
	{ 
		m_SkinningBuffer->Release();
	    m_SkinningBuffer = nullptr;
	}
    if (m_SamplerState)
    {
        m_SamplerState->Release();
        m_SamplerState = nullptr;
    }

    // シャドウマップリソース解放
    // シャドウマップリソース解放
    if (m_ShadowMapTex)  { m_ShadowMapTex->Release();  m_ShadowMapTex  = nullptr; }
    if (m_ShadowDSV)     { m_ShadowDSV->Release();     m_ShadowDSV     = nullptr; }
    if (m_ShadowSRV)     { m_ShadowSRV->Release();     m_ShadowSRV     = nullptr; }
    if (m_ShadowSampler) { m_ShadowSampler->Release(); m_ShadowSampler = nullptr; }
    if (m_ShadowBuffer)  { m_ShadowBuffer->Release();  m_ShadowBuffer  = nullptr; }
    if (m_ShadowRS)      { m_ShadowRS->Release();      m_ShadowRS      = nullptr; }
    if (m_NormalRS)      { m_NormalRS->Release();      m_NormalRS      = nullptr; }
}

// シャドウパス開始
void Renderer::BeginShadowPass(const Vector3& focus)
{
    // ライト視点の行列を計算
    Vector3 lightDir = Vector3(0.5f, -1.3f, 0.8f);
    lightDir.Normalize();

    // 注視点をライト空間でテクセル単位にスナップしてシマー（ちらつき）を防ぐ
    Matrix lightRot = Matrix::CreateLookAt(-lightDir, Vector3::Zero, Vector3::Up);
    Vector3 focusLS = Vector3::Transform(focus, lightRot);
    const float texelSize = SHADOW_ORTHO_SIZE / static_cast<float>(SHADOW_MAP_SIZE);
    focusLS.x = floorf(focusLS.x / texelSize) * texelSize;
    focusLS.y = floorf(focusLS.y / texelSize) * texelSize;
    Vector3 snappedFocus = Vector3::Transform(focusLS, lightRot.Invert());

    // 注視点を中心にした追従正射影（範囲を絞ってテクセル密度を上げる）
    Vector3 lightEye = snappedFocus - lightDir * 3000.0f;
    Matrix lightView = Matrix::CreateLookAt(lightEye, snappedFocus, Vector3::Up);
    Matrix lightProj = Matrix::CreateOrthographic(SHADOW_ORTHO_SIZE, SHADOW_ORTHO_SIZE, 1.0f, 6000.0f);
    Matrix lightViewProj = (lightView * lightProj).Transpose();

    // シャドウ定数バッファ更新 (b8)
    m_DeviceContext->UpdateSubresource(m_ShadowBuffer, 0, nullptr, &lightViewProj, 0, 0);
    m_DeviceContext->VSSetConstantBuffers(8, 1, &m_ShadowBuffer);

    // RTV を外してシャドウマップDSVへ切り替え
    ID3D11RenderTargetView* nullRTV = nullptr;
    m_DeviceContext->OMSetRenderTargets(1, &nullRTV, m_ShadowDSV);
    m_DeviceContext->ClearDepthStencilView(m_ShadowDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);

    // シャドウ用ビューポート設定
    D3D11_VIEWPORT vp{};
    vp.Width    = static_cast<float>(SHADOW_MAP_SIZE);
    vp.Height   = static_cast<float>(SHADOW_MAP_SIZE);
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    m_DeviceContext->RSSetViewports(1, &vp);

    // シャドウ用ラスタライザに切り替え
    m_DeviceContext->RSSetState(m_ShadowRS);
}

// シャドウパス終了
void Renderer::EndShadowPass()
{
    // 通常のRTV/DSVに戻す
    m_DeviceContext->OMSetRenderTargets(1, &m_RenderTargetView, m_DepthStencilView);

    // ビューポートを画面サイズに戻す
    D3D11_VIEWPORT vp{};
    vp.Width    = static_cast<float>(Application::GetWidth());
    vp.Height   = static_cast<float>(Application::GetHeight());
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    m_DeviceContext->RSSetViewports(1, &vp);

    // ラスタライザを通常に戻す
    m_DeviceContext->RSSetState(m_NormalRS);

    // シャドウマップをPSのt1にバインド
    m_DeviceContext->PSSetShaderResources(1, 1, &m_ShadowSRV);
    m_DeviceContext->PSSetSamplers(1, 1, &m_ShadowSampler);

    // ステートキャッシュをリセット
    ResetStateCache();
}

// スタティックメッシュ用シャドウシェーダーをセット
void Renderer::SetShadowStaticShader()
{
    m_ShadowStaticShader.SetGPU();
}

// スキニングメッシュ用シャドウシェーダーをセット
void Renderer::SetShadowSkinnedShader()
{
    m_ShadowSkinnedShader.SetGPU();
}

// スキニングメッシュのシャドウ描画
void Renderer::DrawShadow(const SkinnedModel& model, const Matrix& world)
{
    const auto& mesh  = model.GetMesh();
    const auto& bones = model.GetFinalMatrices();

    // シャドウシェーダーをセット
    SetShadowSkinnedShader();

    // ボーン行列を設定
    SetSkinningMatrices(bones);

    // ワールド行列を設定
    Matrix w = world.Transpose();
    m_DeviceContext->UpdateSubresource(m_WorldBuffer, 0, nullptr, &w, 0, 0);

    // 頂点・インデックスバッファ設定
    UINT stride = sizeof(VERTEX_SKINNED);
    UINT offset = 0;
    m_DeviceContext->IASetVertexBuffers(0, 1, mesh.GetVertexBuffer(), &stride, &offset);
    m_DeviceContext->IASetIndexBuffer(mesh.GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);

    // サブセットごとに描画（テクスチャ不要）
    for (auto& sub : mesh.GetSubsets())
    {
        m_DeviceContext->DrawIndexed(sub.IndexNum, sub.IndexBase, 0);
    }
}

//=======================================
//描画開始
//=======================================
void Renderer::Begin()
{
	// ポストプロセス有効時はHDRシーンRTへ、無効時は従来どおりバックバッファへ描画する
	ID3D11RenderTargetView* sceneRTV = m_RenderTargetView;
	auto& postProcess = PostProcessManager::GetInstance();
	if (postProcess.IsEnabled() && postProcess.GetSceneRTV())
	{
		sceneRTV = postProcess.GetSceneRTV();
	}
	m_DeviceContext->OMSetRenderTargets(1, &sceneRTV, m_DepthStencilView);

	// 既定は不透明
	float blendFactor[4] = { 0,0,0,0 };
	m_DeviceContext->OMSetBlendState(m_BlendState[0], blendFactor, 0xffffffff);

	// 背景を黒に
	const float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	m_DeviceContext->ClearRenderTargetView(sceneRTV, clearColor);
	m_DeviceContext->ClearDepthStencilView(m_DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

    m_DeviceContext->PSSetSamplers(0, 1, &m_SamplerState);

    // 全ライト更新をフレーム先頭で一括送信
    PushLightBuffer();
}

//=======================================
//描画終了
//=======================================
void Renderer::End()
{
    ImGUI_Manager::Render();
	m_SwapChain->Present(1, 0); // VSync 待ちで GPU と同期
}

//=======================================
// 深度ステンシルの有効・無効を設定
//=======================================
void Renderer::SetDepthEnable(bool Enable)
{
	if (Enable)
	{
		// 深度テストを有効にするステンシルステートをセット
		m_DeviceContext->OMSetDepthStencilState(m_DepthStateEnable, NULL);
	}
	else
	{
		// 深度テストを無効にするステンシルステートをセット
		m_DeviceContext->OMSetDepthStencilState(m_DepthStateDisable, NULL);
	}
}

//=======================================
// アルファテストとカバレッジ（ATC）の有効・無効を設定
//=======================================
void Renderer::SetATCEnable(bool Enable)
{
	// ブレンドファクター（透明度などの調整に使用）
	float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	if (Enable)
	{
		// アルファテストとカバレッジ (ATC) を有効にするブレンドステートをセット
		m_DeviceContext->OMSetBlendState(m_BlendStateATC, blendFactor, 0xffffffff);
	}
	else
	{
		// 通常のブレンドステートをセット
		m_DeviceContext->OMSetBlendState(m_BlendState[0], blendFactor, 0xffffffff);
	}
}

//=======================================
// ワールド・ビュー・プロジェクション行列を2D用に設定
//=======================================
void Renderer::SetWorldViewProjection2D()
{
	Matrix world = Matrix::Identity;			// 単位行列にする
	world = world.Transpose();			// 転置
	m_DeviceContext->UpdateSubresource(m_WorldBuffer, 0, NULL, &world, 0, 0);

	Matrix view = Matrix::Identity;			// 単位行列にする
	view = view.Transpose();			// 転置
	m_DeviceContext->UpdateSubresource(m_ViewBuffer, 0, NULL, &view, 0, 0);

	// 2D描画は仮想解像度基準で投影する（実解像度が変わってもUIレイアウトを維持するため）
    Matrix projection = DirectX::XMMatrixOrthographicLH(
        static_cast<float>(Application::VIRTUAL_WIDTH),
        static_cast<float>(Application::VIRTUAL_HEIGHT),
        0.0f,                                          // NearZ
        1.0f);

	projection = projection.Transpose();

	m_DeviceContext->UpdateSubresource(m_ProjectionBuffer, 0, NULL, &projection, 0, 0);
}

//=======================================
// ワールド行列を設定
//=======================================
void Renderer::SetWorldMatrix(Matrix* WorldMatrix)
{
	//Matrix world;
	//world = WorldMatrix->Transpose(); // 転置

	//// ワールド行列をGPU側へ送る
	//m_DeviceContext->UpdateSubresource(m_WorldBuffer, 0, NULL, &world, 0, 0);


	m_WorldMatrix = *WorldMatrix;  // 行列のコピーを保存

	Matrix world = WorldMatrix->Transpose(); // 転置
	// ワールド行列をGPU側へ送る
	m_DeviceContext->UpdateSubresource(m_WorldBuffer, 0, NULL, &world, 0, 0);

    Matrix worldInv = WorldMatrix->Invert();
    m_DeviceContext->UpdateSubresource(m_WorldInverseTransposeBuffer, 0, NULL, &worldInv, 0, 0);
}

//=======================================
// ビュー行列を設定
//=======================================
void Renderer::SetViewMatrix(Matrix* ViewMatrix)
{
	m_ViewMatrix = *ViewMatrix;  // 行列のコピーを保存

	Matrix view = ViewMatrix->Transpose(); // 転置
	// ビュー行列をGPU側へ送る
	m_DeviceContext->UpdateSubresource(m_ViewBuffer, 0, NULL, &view, 0, 0);
}

//=======================================
// プロジェクション行列を設定
//=======================================
void Renderer::SetProjectionMatrix(Matrix* ProjectionMatrix)
{
	//Matrix projection;
	//projection = ProjectionMatrix->Transpose(); // 転置

	//// プロジェクション行列をGPU側へ送る
	//m_DeviceContext->UpdateSubresource(m_ProjectionBuffer, 0, NULL, &projection, 0, 0);

	m_ProjectionMatrix = *ProjectionMatrix;  // 行列のコピーを保存

	Matrix projection = ProjectionMatrix->Transpose(); // 転置
	// プロジェクション行列をGPU側へ送る
	m_DeviceContext->UpdateSubresource(m_ProjectionBuffer, 0, NULL, &projection, 0, 0);
}

//=======================================
// ライト設定
//=======================================
void Renderer::SetLight(LIGHT light)
{
    m_LightData.GlobalLight = light;
    PushLightBuffer();
}

//=======================================
// ノーマルマップ強度の設定・取得
//=======================================
void Renderer::SetNormalMapStrength(float strength)
{
    m_LightData.RenderParams.x = strength;
}

float Renderer::GetNormalMapStrength()
{
    return m_LightData.RenderParams.x;
}

//=======================================
// サンプラーステート設定
//=======================================
void Renderer::BindSampler()
{
    if (m_SamplerState)
    {
        m_DeviceContext->PSSetSamplers(0, 1, &m_SamplerState);
    }
}

//=======================================
// マテリアル設定
//=======================================
void Renderer::SetMaterial(MATERIAL Material) 
{
	m_DeviceContext->UpdateSubresource(m_MaterialBuffer, 0, NULL, &Material, 0, 0);
}

//=======================================
// UV情報を設定
//=======================================
void Renderer::SetUV(float u, float v, float uw, float vh)
{
	Matrix mat = Matrix::CreateScale(uw, vh, 1.0f);
	mat *= Matrix::CreateTranslation(u, v, 0.0f).Transpose();

	m_DeviceContext->UpdateSubresource(m_TextureBuffer, 0, NULL, &mat, 0, 0);
}

//=======================================
//頂点シェーダー作成
//=======================================
void Renderer::CreateVertexShader(ID3D11VertexShader** VertexShader, ID3D11InputLayout** VertexLayout, const char* FileName)
{
	FILE* file; // ファイルを開くためのポインタ
	long int fsize; // ファイルサイズを格納する変数
	fopen_s(&file, FileName, "rb"); // シェーダーファイルをReadBinaryモードで開く
	assert(file); // ファイルが正常に開けたかどうかを確認

	fsize = _filelength(_fileno(file)); // ファイルのサイズを取得
	unsigned char* buffer = new unsigned char[fsize]; // ファイルサイズに基づいてバッファを確保
	fread(buffer, fsize, 1, file); // ファイルからバッファにデータを読み込む
	fclose(file); // 読み込み完了後、ファイルを閉じる

	// デバイスを使って頂点シェーダーを作成
	m_Device->CreateVertexShader(buffer, fsize, NULL, VertexShader);

	// 頂点レイアウト（入力レイアウト）の定義
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		// 頂点の位置情報（3つのfloat値）
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,		0,	0,		D3D11_INPUT_PER_VERTEX_DATA, 0 },
		// 頂点の法線ベクトル情報（3つのfloat値）
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,		0,	4 * 3,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
		// 頂点の色情報（4つのfloat値：RGBA）
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT,	0,	4 * 6,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
		// 頂点のテクスチャ座標情報（2つのfloat値）
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,			0,	4 * 10, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	UINT numElements = ARRAYSIZE(layout); // レイアウトの要素数を取得

	// デバイスを使って頂点レイアウトを作成
	m_Device->CreateInputLayout(layout, numElements, buffer, fsize, VertexLayout);

	delete[] buffer; // バッファのメモリを解放
}

//=======================================
//ピクセルシェーダー作成
//=======================================
void Renderer::CreatePixelShader(ID3D11PixelShader** PixelShader, const char* FileName)
{
	FILE* file; // ファイルを開くためのポインタ
	long int fsize; // ファイルサイズを格納する変数
	fopen_s(&file, FileName, "rb"); // シェーダーファイルをReadBinaryモードで開く
	assert(file); // ファイルが正常に開けたかどうかを確認

	fsize = _filelength(_fileno(file)); // ファイルのサイズを取得
	unsigned char* buffer = new unsigned char[fsize]; // ファイルサイズに基づいてバッファを確保
	fread(buffer, fsize, 1, file); // ファイルからバッファにデータを読み込む
	fclose(file); // 読み込み完了後、ファイルを閉じる

	// デバイスを使ってピクセルシェーダーを作成
	m_Device->CreatePixelShader(buffer, fsize, NULL, PixelShader);

	delete[] buffer; // バッファのメモリを解放
}


// ==========================================================
//点光源の追加
// ==========================================================
int Renderer::AddPointLight(const DirectX::SimpleMath::Vector3& pos, float range, const DirectX::SimpleMath::Color& color)
{
    for (int i = 0; i < MAX_POINT_LIGHTS; ++i)
    {
        // w (alpha) が 0 なら空きスロットとみなす
        if (m_LightData.Lights[i].Color.w <= 0.0f)
        {
            m_LightData.Lights[i].Position = Vector4(pos.x, pos.y, pos.z, range);
            m_LightData.Lights[i].Color = color;
            m_LightData.Lights[i].Color.w = 1.0f; // 有効化

            // 点光源に向きは関係ないが、初期化しておく
            m_LightData.Lights[i].Direction = Vector4(0, -1, 0, 0);

            PushLightBuffer();
            return i; // IDを返す
        }
    }
    return -1; // 空きなし
}




// ==========================================================
//スポットライトの追加
// ==========================================================
int Renderer::AddSpotLight(const DirectX::SimpleMath::Vector3& pos, const DirectX::SimpleMath::Vector3& dir, float range, float angleDeg, const DirectX::SimpleMath::Color& color)
{
    for (int i = 0; i < MAX_POINT_LIGHTS; ++i)
    {
        if (m_LightData.Lights[i].Color.w <= 0.0f)
        {
            m_LightData.Lights[i].Position = Vector4(pos.x, pos.y, pos.z, range);

            // 方向ベクトルを正規化してセット
            Vector3 d = dir;
            d.Normalize();

            // 角度をCos値に変換してwに格納(シェーダーでの計算高速化のため)
            float rad = DirectX::XMConvertToRadians(angleDeg * 0.5f);
            float cosAngle = std::cos(rad);

            m_LightData.Lights[i].Direction = Vector4(d.x, d.y, d.z, cosAngle);

            m_LightData.Lights[i].Color = color;
            m_LightData.Lights[i].Color.w = 2.0f; //スポットライト

            PushLightBuffer();
            return i;
        }
    }
    return -1;
}




// ==========================================================
// ポイントライトの更新
// ==========================================================
void Renderer::UpdatePointLight(int id, const DirectX::SimpleMath::Vector3& pos, float range, const DirectX::SimpleMath::Color& color)
{
    if (id >= 0 && id < MAX_POINT_LIGHTS)
    {
        m_LightData.Lights[id].Position   = Vector4(pos.x, pos.y, pos.z, range);
        m_LightData.Lights[id].Color      = color;
        m_LightData.Lights[id].Color.w    = 1.0f; // point light
        // PushLightBuffer はフレーム先頭の Begin() で一括送信
    }
}




// ==========================================================
// スポットライトの更新
// ==========================================================
void Renderer::UpdateSpotLight(int id, const DirectX::SimpleMath::Vector3& pos, const DirectX::SimpleMath::Vector3& dir, float range, float angleDeg, const DirectX::SimpleMath::Color& color)
{
    // IDが有効範囲内かチェック
    if (id >= 0 && id < MAX_POINT_LIGHTS)
    {
        // 位置と範囲を更新
        m_LightData.Lights[id].Position = Vector4(pos.x, pos.y, pos.z, range);

        // 方向ベクトルを正規化
        Vector3 d = dir;
        d.Normalize();

        // 角度計算 (AddSpotLightと同じ計算式)
        float rad = DirectX::XMConvertToRadians(angleDeg * 0.5f);
        float cosAngle = std::cos(rad);

        // 向きと角度制限を更新
        m_LightData.Lights[id].Direction = Vector4(d.x, d.y, d.z, cosAngle);

        // 色とタイプを更新 (w=2.0f はスポットライト)
        m_LightData.Lights[id].Color = color;
        m_LightData.Lights[id].Color.w = 2.0f;
        // PushLightBuffer はフレーム先頭の Begin() で一括送信
    }
}



// ==========================================================
//光源の削除
// ==========================================================
void Renderer::RemoveLight(int id)
{
    if (id >= 0 && id < MAX_POINT_LIGHTS)
    {
        m_LightData.Lights[id].Color.w = 0.0f; // 無効化
        // PushLightBuffer はフレーム先頭の Begin() で一括送信
    }
}

// ==========================================================
//光源の全削除
// ==========================================================
void Renderer::ClearLights()
{
    for (int i = 0; i < MAX_POINT_LIGHTS; ++i)
    {
        m_LightData.Lights[i].Color = Color(0, 0, 0, 0.0f);
    }
    PushLightBuffer();
}

// ==========================================================
//ライトバッファのGPU送信
// ==========================================================
void Renderer::PushLightBuffer()
{
    if (m_DeviceContext && m_LightBuffer)
    {
        Matrix invView = m_ViewMatrix.Invert();
        m_LightData.GlobalLight.CameraPos = invView.Translation();

        m_DeviceContext->UpdateSubresource(m_LightBuffer, 0, NULL, &m_LightData, 0, 0);
    }
}

void Renderer::BindLitCommonCB()
{
    // VS側
    m_DeviceContext->VSSetConstantBuffers(0, 1, &m_WorldBuffer);
    m_DeviceContext->VSSetConstantBuffers(1, 1, &m_ViewBuffer);
    m_DeviceContext->VSSetConstantBuffers(2, 1, &m_ProjectionBuffer);
    m_DeviceContext->VSSetConstantBuffers(3, 1, &m_LightBuffer);
    m_DeviceContext->VSSetConstantBuffers(4, 1, &m_MaterialBuffer);
    m_DeviceContext->VSSetConstantBuffers(5, 1, &m_TextureBuffer);
    m_DeviceContext->VSSetConstantBuffers(7, 1, &m_WorldInverseTransposeBuffer);

    // PS側
    m_DeviceContext->PSSetConstantBuffers(3, 1, &m_LightBuffer);
    m_DeviceContext->PSSetConstantBuffers(4, 1, &m_MaterialBuffer);
	m_DeviceContext->PSSetConstantBuffers(5, 1, &m_UVAdjustBuffer);

    // サンプラ
    m_DeviceContext->PSSetSamplers(0, 1, &m_SamplerState);
}
