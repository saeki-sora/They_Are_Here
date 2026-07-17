#include "pch.h"
#include "PostProcessManager.h"
#include "Application.h"
#include "dx11helper.h"
#include "imgui.h"
#include "json.hpp"

using json = nlohmann::json;

// グラフィック設定の保存先
static constexpr const char* GRAPHICS_SETTINGS_PATH = "json/graphics_param.json";

using namespace DirectX::SimpleMath;
using Microsoft::WRL::ComPtr;

// インスタンスを取得
PostProcessManager& PostProcessManager::GetInstance()
{
	static PostProcessManager instance;
	return instance;
}

// 初期化
void PostProcessManager::Init()
{
	ID3D11Device* device = Renderer::GetDevice();
	if (!device) return;

	const UINT width = Application::GetWidth();
	const UINT height = Application::GetHeight();

	// --- レンダーターゲット作成 ---
	// HDRシーンRT（フル解像度）と FXAA入力用LDR中間RT
	if (!CreateRenderTarget(m_SceneRT, width, height, DXGI_FORMAT_R16G16B16A16_FLOAT)) return;
	if (!CreateRenderTarget(m_LDRRT, width, height, DXGI_FORMAT_R8G8B8A8_UNORM)) return;

	// ブルームチェーン（輝度抽出1/2 → Mip 1/4, 1/8, 1/16）
	if (!CreateRenderTarget(m_BloomExtractRT, width / 2, height / 2, DXGI_FORMAT_R16G16B16A16_FLOAT)) return;
	for (int i = 0; i < BLOOM_MIP_COUNT; ++i)
	{
		UINT mipW = width >> (2 + i);
		UINT mipH = height >> (2 + i);
		if (!CreateRenderTarget(m_BloomRT[i], mipW, mipH, DXGI_FORMAT_R16G16B16A16_FLOAT)) return;
		if (!CreateRenderTarget(m_BloomWorkRT[i], mipW, mipH, DXGI_FORMAT_R16G16B16A16_FLOAT)) return;
	}

	// SSAO・ボリュメトリック（半解像度）
	if (!CreateRenderTarget(m_SSAORT, width / 2, height / 2, DXGI_FORMAT_R8_UNORM)) return;
	if (!CreateRenderTarget(m_SSAOWorkRT, width / 2, height / 2, DXGI_FORMAT_R8_UNORM)) return;
	if (!CreateRenderTarget(m_VolumetricRT, width / 2, height / 2, DXGI_FORMAT_R16G16B16A16_FLOAT)) return;
	if (!CreateRenderTarget(m_VolumetricWorkRT, width / 2, height / 2, DXGI_FORMAT_R16G16B16A16_FLOAT)) return;

	// --- シェーダー作成 ---
	// フルスクリーンVS（入力レイアウト不要のためdx11helperを直接使う）
	{
		ComPtr<ID3DBlob> blob;
		HRESULT hr = CompileShaderFromFile("shader/FullScreenVS.hlsl", "vs_main", "vs_5_0", &blob);
		if (FAILED(hr)) return;
		hr = device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &m_FullScreenVS);
		if (FAILED(hr)) return;
	}
	if (!LoadPixelShader(m_CompositePS, "shader/CompositePS.hlsl")) return;
	if (!LoadPixelShader(m_FXAAPS, "shader/FXAAPS.hlsl")) return;
	if (!LoadPixelShader(m_BrightExtractPS, "shader/BrightExtractPS.hlsl")) return;
	if (!LoadPixelShader(m_DownsamplePS, "shader/DownsamplePS.hlsl")) return;
	if (!LoadPixelShader(m_GaussBlurPS, "shader/GaussBlurPS.hlsl")) return;
	if (!LoadPixelShader(m_SSAOPS, "shader/SSAOPS.hlsl")) return;
	if (!LoadPixelShader(m_BilateralBlurPS, "shader/BilateralBlurPS.hlsl")) return;
	if (!LoadPixelShader(m_VolumetricPS, "shader/VolumetricPS.hlsl")) return;

	// --- 定数バッファ作成 ---
	if (!CreateConstantBuffer(device, sizeof(POST_PROCESS_BUFFER), &m_PostBuffer)) return;
	if (!CreateConstantBuffer(device, sizeof(VOLUMETRIC_LIGHT_BUFFER), &m_VolumetricBuffer)) return;

	// --- サンプラー作成 ---
	{
		D3D11_SAMPLER_DESC sd{};
		sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		sd.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		sd.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		sd.MaxLOD = D3D11_FLOAT32_MAX;
		if (FAILED(device->CreateSamplerState(&sd, &m_LinearClampSampler))) return;

		sd.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
		if (FAILED(device->CreateSamplerState(&sd, &m_PointClampSampler))) return;
	}

	m_IsInitialized = true;

	// 前回保存した設定があれば読み込む（無ければ既定値のまま）
	LoadSettings(GRAPHICS_SETTINGS_PATH);
}

// 現在のグラフィック設定をJSONファイルへ保存する
void PostProcessManager::SaveSettings(const std::string& filePath)
{
	json data;
	data["Enabled"] = m_Enabled;
	data["TonemapBypass"] = m_TonemapBypass;
	data["BloomEnabled"] = m_BloomEnabled;
	data["FXAAEnabled"] = m_FXAAEnabled;
	data["SSAOEnabled"] = m_SSAOEnabled;
	data["VolumetricEnabled"] = m_VolumetricEnabled;
	data["Exposure"] = m_Exposure;
	data["Saturation"] = m_Saturation;
	data["BloomThreshold"] = m_BloomThreshold;
	data["BloomKnee"] = m_BloomKnee;
	data["BloomIntensity"] = m_BloomIntensity;
	data["VignetteIntensity"] = m_VignetteIntensity;
	data["VignettePower"] = m_VignettePower;
	data["GrainIntensity"] = m_GrainIntensity;
	data["TintStrength"] = m_TintStrength;
	data["TintColor"] = { m_TintColor.x, m_TintColor.y, m_TintColor.z };
	data["SSAORadius"] = m_SSAORadius;
	data["SSAOIntensity"] = m_SSAOIntensity;
	data["SSAOBias"] = m_SSAOBias;
	data["VolumetricIntensity"] = m_VolumetricIntensity;
	data["VolumetricSteps"] = m_VolumetricSteps;
	data["VolumetricMaxDist"] = m_VolumetricMaxDist;
	data["NormalMapStrength"] = Renderer::GetNormalMapStrength();

	std::ofstream file(filePath);
	if (!file.is_open())
	{
		std::cerr << "[PostProcessManager] Error: 書き込めません: " << filePath << std::endl;
		return;
	}
	file << data.dump(2);

#ifdef _DEBUG
	std::cout << "[PostProcessManager] グラフィック設定を保存しました: " << filePath << std::endl;
#endif
}

// JSONファイルからグラフィック設定を読み込む
void PostProcessManager::LoadSettings(const std::string& filePath)
{
	std::ifstream file(filePath);
	if (!file.is_open()) return; // 保存ファイルが無い場合は既定値のまま

	json data;
	try
	{
		file >> data;
	}
	catch (json::parse_error& e)
	{
		std::cerr << "[PostProcessManager] JSONパースエラー: " << e.what() << std::endl;
		return;
	}

	m_Enabled = data.value("Enabled", m_Enabled);
	m_TonemapBypass = data.value("TonemapBypass", m_TonemapBypass);
	m_BloomEnabled = data.value("BloomEnabled", m_BloomEnabled);
	m_FXAAEnabled = data.value("FXAAEnabled", m_FXAAEnabled);
	m_SSAOEnabled = data.value("SSAOEnabled", m_SSAOEnabled);
	m_VolumetricEnabled = data.value("VolumetricEnabled", m_VolumetricEnabled);
	m_Exposure = data.value("Exposure", m_Exposure);
	m_Saturation = data.value("Saturation", m_Saturation);
	m_BloomThreshold = data.value("BloomThreshold", m_BloomThreshold);
	m_BloomKnee = data.value("BloomKnee", m_BloomKnee);
	m_BloomIntensity = data.value("BloomIntensity", m_BloomIntensity);
	m_VignetteIntensity = data.value("VignetteIntensity", m_VignetteIntensity);
	m_VignettePower = data.value("VignettePower", m_VignettePower);
	m_GrainIntensity = data.value("GrainIntensity", m_GrainIntensity);
	m_TintStrength = data.value("TintStrength", m_TintStrength);

	if (data.contains("TintColor") && data["TintColor"].is_array() && data["TintColor"].size() == 3)
	{
		m_TintColor = Vector3(
			data["TintColor"][0].get<float>(),
			data["TintColor"][1].get<float>(),
			data["TintColor"][2].get<float>());
	}

	m_SSAORadius = data.value("SSAORadius", m_SSAORadius);
	m_SSAOIntensity = data.value("SSAOIntensity", m_SSAOIntensity);
	m_SSAOBias = data.value("SSAOBias", m_SSAOBias);
	m_VolumetricIntensity = data.value("VolumetricIntensity", m_VolumetricIntensity);
	m_VolumetricSteps = data.value("VolumetricSteps", m_VolumetricSteps);
	m_VolumetricMaxDist = data.value("VolumetricMaxDist", m_VolumetricMaxDist);

	float normalStrength = data.value("NormalMapStrength", Renderer::GetNormalMapStrength());
	Renderer::SetNormalMapStrength(normalStrength);

#ifdef _DEBUG
	std::cout << "[PostProcessManager] グラフィック設定を読み込みました: " << filePath << std::endl;
#endif
}

// 終了処理（ComPtrなので明示解放は不要）
void PostProcessManager::Uninit()
{
	m_IsInitialized = false;
}

// 更新（グレインのシード用に時間を進める）
void PostProcessManager::Update(float deltaTime)
{
	m_Time += deltaTime;
	if (m_Time > 3600.0f) m_Time = 0.0f;
}

// ポストプロセス全体が有効か
bool PostProcessManager::IsEnabled() const
{
	return m_Enabled && m_IsInitialized;
}

// HDRシーンレンダーターゲットを取得
ID3D11RenderTargetView* PostProcessManager::GetSceneRTV() const
{
	return m_SceneRT.RTV.Get();
}

// RT作成ヘルパー
bool PostProcessManager::CreateRenderTarget(RenderTarget& rt, UINT width, UINT height, DXGI_FORMAT format)
{
	ID3D11Device* device = Renderer::GetDevice();
	if (width == 0) width = 1;
	if (height == 0) height = 1;

	D3D11_TEXTURE2D_DESC desc{};
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = format;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

	if (FAILED(device->CreateTexture2D(&desc, nullptr, &rt.Tex))) return false;
	if (FAILED(device->CreateRenderTargetView(rt.Tex.Get(), nullptr, &rt.RTV))) return false;
	if (FAILED(device->CreateShaderResourceView(rt.Tex.Get(), nullptr, &rt.SRV))) return false;

	rt.Width = width;
	rt.Height = height;
	return true;
}

// ピクセルシェーダーのロードヘルパー
bool PostProcessManager::LoadPixelShader(ComPtr<ID3D11PixelShader>& ps, const char* fileName)
{
	ComPtr<ID3DBlob> blob;
	HRESULT hr = CompileShaderFromFile(fileName, "ps_main", "ps_5_0", &blob);
	if (FAILED(hr)) return false;

	hr = Renderer::GetDevice()->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &ps);
	return SUCCEEDED(hr);
}

// 定数バッファの共通部分を更新する
void PostProcessManager::UpdateConstantBuffer()
{
	const float width = static_cast<float>(Application::GetWidth());
	const float height = static_cast<float>(Application::GetHeight());

	// 行列はシーン描画時のものを使用（HLSL側は mul(v, M) 前提なので転置して送る）
	Matrix view = Renderer::GetViewMatrix();
	Matrix proj = Renderer::GetProjectionMatrix();
	Matrix invViewProj = (view * proj).Invert();

	m_CBData.InvViewProj = invViewProj.Transpose();
	m_CBData.InvProjection = proj.Invert().Transpose();
	m_CBData.Projection = proj.Transpose();
	m_CBData.ScreenInfo = Vector4(width, height, 1.0f / width, 1.0f / height);
	m_CBData.ToneParams = Vector4(m_Exposure, m_TonemapBypass ? 1.0f : 0.0f, m_Saturation, m_Time);
	m_CBData.BloomParams = Vector4(m_BloomThreshold, m_BloomKnee, m_BloomEnabled ? m_BloomIntensity : 0.0f, 0.0f);
	m_CBData.BloomWeights = Vector4(0.5f, 0.3f, 0.2f, 0.0f);
	m_CBData.BlurDir = Vector4(1.0f, 0.0f, 0.0f, 0.0f);
	m_CBData.VignetteParams = Vector4(m_VignetteIntensity, m_VignettePower, m_GrainIntensity, 0.0f);
	m_CBData.ColorTint = Vector4(m_TintColor.x, m_TintColor.y, m_TintColor.z, m_TintStrength);
	m_CBData.SSAOParams = Vector4(m_SSAORadius, m_SSAOIntensity, m_SSAOBias, m_SSAOEnabled ? 1.0f : 0.0f);
	m_CBData.VolumetricParams = Vector4(m_VolumetricIntensity, static_cast<float>(m_VolumetricSteps), m_VolumetricMaxDist, m_VolumetricEnabled ? 1.0f : 0.0f);
	m_CBData.CameraParams = Vector4(0.1f, 10000.0f, 0.0f, 0.0f); // Camera::SetCamera のニア/ファーと合わせる

	// カメラ位置はビュー行列の逆行列から取得する
	Vector3 camPos = view.Invert().Translation();
	m_CBData.CameraPos = Vector4(camPos.x, camPos.y, camPos.z, 0.0f);
}

// 定数バッファをGPUへ転送する
void PostProcessManager::PushConstantBuffer()
{
	ID3D11DeviceContext* context = Renderer::GetDeviceContext();
	context->UpdateSubresource(m_PostBuffer.Get(), 0, nullptr, &m_CBData, 0, 0);
}

// フルスクリーン三角形を出力先RTへ描画する
void PostProcessManager::DrawFullScreen(ID3D11RenderTargetView* rtv, ID3D11PixelShader* ps, UINT width, UINT height)
{
	ID3D11DeviceContext* context = Renderer::GetDeviceContext();

	// 出力先サイズに合わせたビューポート
	D3D11_VIEWPORT vp{};
	vp.Width = static_cast<float>(width);
	vp.Height = static_cast<float>(height);
	vp.MaxDepth = 1.0f;
	context->RSSetViewports(1, &vp);

	context->OMSetRenderTargets(1, &rtv, nullptr);
	context->PSSetShader(ps, nullptr, 0);
	context->Draw(3, 0);

	// 描画後はRTVを外す（このRTを次パスでSRVとしてバインドした際に
	// ランタイムのハザード保護でnull化されるのを防ぐ）
	ID3D11RenderTargetView* nullRTV = nullptr;
	context->OMSetRenderTargets(1, &nullRTV, nullptr);
}

// SSAOパス（半解像度AO計算 → バイラテラルブラー縦横）
void PostProcessManager::ExecuteSSAO()
{
	ID3D11DeviceContext* context = Renderer::GetDeviceContext();
	const UINT w = m_SSAORT.Width;
	const UINT h = m_SSAORT.Height;

	// 半解像度パス中は 1/サイズ をAO解像度に合わせる
	m_CBData.ScreenInfo = Vector4(static_cast<float>(w), static_cast<float>(h), 1.0f / w, 1.0f / h);
	PushConstantBuffer();

	// AO計算（t1=深度）
	DrawFullScreen(m_SSAORT.RTV.Get(), m_SSAOPS.Get(), w, h);

	// バイラテラルブラー横
	m_CBData.BlurDir = Vector4(1.0f, 0.0f, 0.0f, 0.0f);
	PushConstantBuffer();
	ID3D11ShaderResourceView* srv = m_SSAORT.SRV.Get();
	context->PSSetShaderResources(0, 1, &srv);
	DrawFullScreen(m_SSAOWorkRT.RTV.Get(), m_BilateralBlurPS.Get(), w, h);

	// バイラテラルブラー縦（SRV/RTVを入れ替える前に一旦アンバインド）
	ID3D11ShaderResourceView* nullSRV = nullptr;
	context->PSSetShaderResources(0, 1, &nullSRV);
	m_CBData.BlurDir = Vector4(0.0f, 1.0f, 0.0f, 0.0f);
	PushConstantBuffer();
	srv = m_SSAOWorkRT.SRV.Get();
	context->PSSetShaderResources(0, 1, &srv);
	DrawFullScreen(m_SSAORT.RTV.Get(), m_BilateralBlurPS.Get(), w, h);
	context->PSSetShaderResources(0, 1, &nullSRV);
}

// ブルームパス（輝度抽出 → ダウンサンプルチェーン → 各Mipをガウスブラー）
void PostProcessManager::ExecuteBloom()
{
	ID3D11DeviceContext* context = Renderer::GetDeviceContext();
	ID3D11ShaderResourceView* nullSRV = nullptr;

	// 輝度抽出（1/2解像度、入力はHDRシーン）
	ID3D11ShaderResourceView* srv = m_SceneRT.SRV.Get();
	context->PSSetShaderResources(0, 1, &srv);
	DrawFullScreen(m_BloomExtractRT.RTV.Get(), m_BrightExtractPS.Get(), m_BloomExtractRT.Width, m_BloomExtractRT.Height);

	// ダウンサンプルチェーン（1/2 → 1/4 → 1/8 → 1/16）
	const RenderTarget* prev = &m_BloomExtractRT;
	for (int i = 0; i < BLOOM_MIP_COUNT; ++i)
	{
		context->PSSetShaderResources(0, 1, &nullSRV);
		srv = prev->SRV.Get();
		context->PSSetShaderResources(0, 1, &srv);
		DrawFullScreen(m_BloomRT[i].RTV.Get(), m_DownsamplePS.Get(), m_BloomRT[i].Width, m_BloomRT[i].Height);
		prev = &m_BloomRT[i];
	}

	// 各Mipを横→縦の分離ガウスでぼかす
	for (int i = 0; i < BLOOM_MIP_COUNT; ++i)
	{
		const UINT w = m_BloomRT[i].Width;
		const UINT h = m_BloomRT[i].Height;
		m_CBData.ScreenInfo = Vector4(static_cast<float>(w), static_cast<float>(h), 1.0f / w, 1.0f / h);

		// 横ブラー
		m_CBData.BlurDir = Vector4(1.0f, 0.0f, 0.0f, 0.0f);
		PushConstantBuffer();
		context->PSSetShaderResources(0, 1, &nullSRV);
		srv = m_BloomRT[i].SRV.Get();
		context->PSSetShaderResources(0, 1, &srv);
		DrawFullScreen(m_BloomWorkRT[i].RTV.Get(), m_GaussBlurPS.Get(), w, h);

		// 縦ブラー
		m_CBData.BlurDir = Vector4(0.0f, 1.0f, 0.0f, 0.0f);
		PushConstantBuffer();
		context->PSSetShaderResources(0, 1, &nullSRV);
		srv = m_BloomWorkRT[i].SRV.Get();
		context->PSSetShaderResources(0, 1, &srv);
		DrawFullScreen(m_BloomRT[i].RTV.Get(), m_GaussBlurPS.Get(), w, h);
	}
	context->PSSetShaderResources(0, 1, &nullSRV);
}

// ボリュメトリックライトパス（カメラ近傍のライトを抽出してレイマーチ → ブラー）
void PostProcessManager::ExecuteVolumetric()
{
	ID3D11DeviceContext* context = Renderer::GetDeviceContext();

	// シーンの動的ライトからカメラに近い順に最大8灯を抽出する
	const LIGHT_CONSTANT_BUFFER& lightData = Renderer::GetLightData();
	Vector3 camPos(m_CBData.CameraPos.x, m_CBData.CameraPos.y, m_CBData.CameraPos.z);

	// 有効ライトのインデックスと距離を集める
	std::vector<std::pair<float, int>> candidates;
	candidates.reserve(MAX_POINT_LIGHTS);
	for (int i = 0; i < MAX_POINT_LIGHTS; ++i)
	{
		if (lightData.Lights[i].Color.w <= 0.0f) continue;
		Vector3 lightPos(lightData.Lights[i].Position.x, lightData.Lights[i].Position.y, lightData.Lights[i].Position.z);
		candidates.emplace_back(Vector3::DistanceSquared(camPos, lightPos), i);
	}
	std::sort(candidates.begin(), candidates.end());

	VOLUMETRIC_LIGHT_BUFFER volData{};
	const int count = std::min(static_cast<int>(candidates.size()), MAX_VOLUMETRIC_LIGHTS);
	for (int i = 0; i < count; ++i)
	{
		volData.Lights[i] = lightData.Lights[candidates[i].second];
	}
	context->UpdateSubresource(m_VolumetricBuffer.Get(), 0, nullptr, &volData, 0, 0);
	context->PSSetConstantBuffers(10, 1, m_VolumetricBuffer.GetAddressOf());

	const UINT w = m_VolumetricRT.Width;
	const UINT h = m_VolumetricRT.Height;
	m_CBData.ScreenInfo = Vector4(static_cast<float>(w), static_cast<float>(h), 1.0f / w, 1.0f / h);
	PushConstantBuffer();

	// レイマーチ（t1=深度）
	DrawFullScreen(m_VolumetricRT.RTV.Get(), m_VolumetricPS.Get(), w, h);

	// ガウスブラー横→縦でディザノイズを均す
	ID3D11ShaderResourceView* nullSRV = nullptr;
	m_CBData.BlurDir = Vector4(1.0f, 0.0f, 0.0f, 0.0f);
	PushConstantBuffer();
	ID3D11ShaderResourceView* srv = m_VolumetricRT.SRV.Get();
	context->PSSetShaderResources(0, 1, &srv);
	DrawFullScreen(m_VolumetricWorkRT.RTV.Get(), m_GaussBlurPS.Get(), w, h);

	context->PSSetShaderResources(0, 1, &nullSRV);
	m_CBData.BlurDir = Vector4(0.0f, 1.0f, 0.0f, 0.0f);
	PushConstantBuffer();
	srv = m_VolumetricWorkRT.SRV.Get();
	context->PSSetShaderResources(0, 1, &srv);
	DrawFullScreen(m_VolumetricRT.RTV.Get(), m_GaussBlurPS.Get(), w, h);
	context->PSSetShaderResources(0, 1, &nullSRV);
}

// 最終合成パス（Composite → FXAA → バックバッファ）
void PostProcessManager::ExecuteComposite()
{
	ID3D11DeviceContext* context = Renderer::GetDeviceContext();
	const UINT width = Application::GetWidth();
	const UINT height = Application::GetHeight();

	// フル解像度に戻す
	m_CBData.ScreenInfo = Vector4(static_cast<float>(width), static_cast<float>(height), 1.0f / width, 1.0f / height);

	// AOデバッグ表示時はSSAO結果をそのまま見せるため他エフェクトを切る
	if (m_SSAODebugView)
	{
		m_CBData.BloomParams.z = 0.0f;
		m_CBData.VolumetricParams.w = 0.0f;
	}
	PushConstantBuffer();

	// 入力SRVを一括バインド（t0:シーン t2-t4:ブルーム t5:AO t6:ボリュメトリック）
	ID3D11ShaderResourceView* srvs[7] = {
		m_SSAODebugView ? m_SSAORT.SRV.Get() : m_SceneRT.SRV.Get(),
		nullptr,
		m_BloomRT[0].SRV.Get(),
		m_BloomRT[1].SRV.Get(),
		m_BloomRT[2].SRV.Get(),
		m_SSAORT.SRV.Get(),
		m_VolumetricRT.SRV.Get(),
	};
	context->PSSetShaderResources(0, 7, srvs);

	// FXAA有効時はLDR中間RTへ、無効時は直接バックバッファへ出力する
	ID3D11RenderTargetView* backBuffer = Renderer::GetRenderTargetView();
	ID3D11RenderTargetView* compositeTarget = m_FXAAEnabled ? m_LDRRT.RTV.Get() : backBuffer;
	DrawFullScreen(compositeTarget, m_CompositePS.Get(), width, height);

	// FXAAパス
	if (m_FXAAEnabled)
	{
		ID3D11ShaderResourceView* nullSRV = nullptr;
		context->PSSetShaderResources(0, 1, &nullSRV);
		ID3D11ShaderResourceView* srv = m_LDRRT.SRV.Get();
		context->PSSetShaderResources(0, 1, &srv);
		DrawFullScreen(backBuffer, m_FXAAPS.Get(), width, height);
	}
}

// 描画ステートを通常描画用へ復帰させる
void PostProcessManager::RestoreRenderState()
{
	ID3D11DeviceContext* context = Renderer::GetDeviceContext();

	// 入力SRVを全て外す（次フレームでHDR RTを再びRTVにするため必須）
	ID3D11ShaderResourceView* nullSRVs[7] = {};
	context->PSSetShaderResources(0, 7, nullSRVs);

	// バックバッファ + 既存深度に戻す（以降のUI描画用）
	ID3D11RenderTargetView* backBuffer = Renderer::GetRenderTargetView();
	context->OMSetRenderTargets(1, &backBuffer, Renderer::GetDepthStencilView());

	// ビューポートをフル解像度へ戻す
	D3D11_VIEWPORT vp{};
	vp.Width = static_cast<float>(Application::GetWidth());
	vp.Height = static_cast<float>(Application::GetHeight());
	vp.MaxDepth = 1.0f;
	context->RSSetViewports(1, &vp);

	// シーン用サンプラー(s0)とシェーダーキャッシュを復帰させる
	Renderer::BindSampler();
	Renderer::ResetStateCache();
}

// ポストプロセスチェーンの実行
void PostProcessManager::Execute()
{
	if (!IsEnabled()) return;

	ID3D11DeviceContext* context = Renderer::GetDeviceContext();

	// 深度をSRVとして読むため、先にRT/DSVのバインドを全て外す
	ID3D11RenderTargetView* nullRTV = nullptr;
	context->OMSetRenderTargets(1, &nullRTV, nullptr);

	// ポスト用の共通ステートを設定（VS・トポロジ・サンプラー・ブレンド）
	context->IASetInputLayout(nullptr);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->VSSetShader(m_FullScreenVS.Get(), nullptr, 0);
	context->OMSetBlendState(nullptr, nullptr, 0xffffffff);
	context->PSSetSamplers(0, 1, m_LinearClampSampler.GetAddressOf());
	context->PSSetSamplers(1, 1, m_PointClampSampler.GetAddressOf());
	context->PSSetConstantBuffers(9, 1, m_PostBuffer.GetAddressOf());

	// 深度SRVをt1へバインド（SSAO・ボリュメトリックで使用）
	ID3D11ShaderResourceView* depthSRV = Renderer::GetDepthSRV();
	context->PSSetShaderResources(1, 1, &depthSRV);

	// 定数バッファ更新
	UpdateConstantBuffer();
	PushConstantBuffer();

	// 各エフェクトパス
	if (m_SSAOEnabled)
	{
		ExecuteSSAO();
	}
	if (m_VolumetricEnabled)
	{
		ExecuteVolumetric();
	}
	if (m_BloomEnabled)
	{
		ExecuteBloom();
	}
	ExecuteComposite();

	// ステートを通常描画用へ戻す
	RestoreRenderState();
}

// ImGuiデバッグパネルの中身を描画する
void PostProcessManager::DrawImGui()
{
	ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
	ImGui::Checkbox("ポストプロセス有効", &m_Enabled);
	if (!m_Enabled) return;

	if (ImGui::CollapsingHeader("トーンマップ / カラー", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Checkbox("トーンマップ無効化", &m_TonemapBypass);
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("ONにするとトーンマップ処理を素通しにする");
		ImGui::SliderFloat("露出", &m_Exposure, 0.1f, 4.0f);
		ImGui::SliderFloat("彩度", &m_Saturation, 0.0f, 2.0f);
		ImGui::SliderFloat("色調の強さ", &m_TintStrength, 0.0f, 1.0f);
		float tint[3] = { m_TintColor.x, m_TintColor.y, m_TintColor.z };
		if (ImGui::ColorEdit3("色調カラー", tint))
		{
			m_TintColor = Vector3(tint[0], tint[1], tint[2]);
		}
		ImGui::SliderFloat("ビネット強度", &m_VignetteIntensity, 0.0f, 1.0f);
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("画面端を暗くする効果の強さ");
		ImGui::SliderFloat("ビネット減衰率", &m_VignettePower, 0.5f, 4.0f);
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("画面端が暗くなり始める範囲の広さ");
		ImGui::SliderFloat("フィルムグレイン", &m_GrainIntensity, 0.0f, 0.2f);
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("画面にノイズ（粒状感）を重ねる強さ");
	}

	if (ImGui::CollapsingHeader("ブルーム", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Checkbox("ブルーム有効", &m_BloomEnabled);
		ImGui::SliderFloat("しきい値", &m_BloomThreshold, 0.0f, 3.0f);
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("この明るさを超えた部分だけ発光させる");
		ImGui::SliderFloat("ソフトニー", &m_BloomKnee, 0.01f, 1.0f);
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("しきい値付近の輝度をなめらかに繋げる度合い");
		ImGui::SliderFloat("強度", &m_BloomIntensity, 0.0f, 3.0f);
	}

	if (ImGui::CollapsingHeader("SSAO", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Checkbox("SSAO有効", &m_SSAOEnabled);
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("物体の隙間・接地面を暗くする環境遮蔽効果");
		ImGui::SliderFloat("半径", &m_SSAORadius, 5.0f, 150.0f);
		ImGui::SliderFloat("SSAO強度", &m_SSAOIntensity, 0.0f, 4.0f);
		ImGui::SliderFloat("バイアス", &m_SSAOBias, 0.0f, 0.2f);
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("平坦な面が誤って暗くなるのを防ぐための補正値");
		ImGui::Checkbox("AOデバッグ表示", &m_SSAODebugView);
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("遮蔽の効果だけを白黒で確認表示する");
	}

	if (ImGui::CollapsingHeader("ボリュームライト", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Checkbox("ボリュームライト有効", &m_VolumetricEnabled);
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("光が空気中で散乱して見える光条（光の筋）の表現");
		ImGui::SliderFloat("散乱強度", &m_VolumetricIntensity, 0.0f, 5.0f);
		ImGui::SliderInt("ステップ数", &m_VolumetricSteps, 8, 32);
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("多いほど滑らかになるが負荷が上がる");
		ImGui::SliderFloat("最大距離", &m_VolumetricMaxDist, 200.0f, 3000.0f);
	}

	if (ImGui::CollapsingHeader("FXAA", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Checkbox("FXAA有効", &m_FXAAEnabled);
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("輪郭のギザギザを滑らかにするアンチエイリアス処理");
	}

	if (ImGui::CollapsingHeader("表面", ImGuiTreeNodeFlags_DefaultOpen))
	{
		float normalStrength = Renderer::GetNormalMapStrength();
		if (ImGui::SliderFloat("法線マップ強度", &normalStrength, 0.0f, 3.0f))
		{
			Renderer::SetNormalMapStrength(normalStrength);
		}
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("表面の凹凸表現（ノーマルマップ）の強さ");
	}

	ImGui::Separator();

	// 保存・読み込み完了メッセージの表示期限（BeginPopupは外側クリックしないと消えないため、
	// 一定時間で自動的に消えるタイマー表示に変更している）
	static double s_SaveMsgUntil = 0.0;
	static double s_LoadMsgUntil = 0.0;
	const double MESSAGE_DURATION = 1.5;

	// 現在の設定をJSONファイルに保存
	if (ImGui::Button("JSONに保存", ImVec2(-1, 0)))
	{
		SaveSettings(GRAPHICS_SETTINGS_PATH);
		s_SaveMsgUntil = ImGui::GetTime() + MESSAGE_DURATION;
	}
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("現在の値を graphics_param.json に書き込む");

	if (ImGui::GetTime() < s_SaveMsgUntil)
		ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "graphics_param.json に保存しました");

	ImGui::Spacing();

	// 保存済みのJSONファイルから設定を読み直す
	if (ImGui::Button("JSONを読み込む", ImVec2(-1, 0)))
	{
		LoadSettings(GRAPHICS_SETTINGS_PATH);
		s_LoadMsgUntil = ImGui::GetTime() + MESSAGE_DURATION;
	}
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("graphics_param.json から値を再読み込みする");

	if (ImGui::GetTime() < s_LoadMsgUntil)
		ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "graphics_param.json から読み込みました");
}
