#include "pch.h"
#include "MiniMap.h"
#include "SceneManager.h"
#include "Input.h"
#include "Renderer.h"
#include "MakeMap.h"
#include "Player.h"
#include "Application.h"


using namespace DirectX;
using namespace DirectX::SimpleMath;



static void SetupFlatMaterial()
{
	MATERIAL mat = {};
	mat.Ambient = DirectX::SimpleMath::Color(1.0f, 1.0f, 1.0f, 1.0f); // 環墁E�EMAX
	mat.Emission = DirectX::SimpleMath::Color(1.0f, 1.0f, 1.0f, 1.0f); // 発光MAX
	mat.Diffuse = DirectX::SimpleMath::Color(0.0f, 0.0f, 0.0f, 0.0f); // 拡散光なぁE
	mat.TextureEnable = TRUE;
	Renderer::SetMaterial(mat);
}

//=============================================================================
// 定数・設宁E
//=============================================================================

namespace {

	// チE��スチャ設宁E
	constexpr int   kTexSize = 512;

	// UI表示設宁E
	constexpr float kSmallMapSize = 250.0f;
	constexpr float kLargeMapScreenRate = 0.8f;
	constexpr float kFrameInnerRate = 0.7f;
	constexpr float kMapMargin = 20.0f;
	constexpr float kZoomLevel = 0.3f; // 拡大時�E表示篁E���E�E.0-1.0�E�E

	// アイコンサイズ
	constexpr float kPlayerIconSize = 32.0f;

	// UV反転設宁E
	constexpr bool  kFlipMiniMapU = false;
	constexpr bool  kFlipMiniMapV = true;
	constexpr bool  kQuadFlipU = false;
	constexpr bool  kQuadFlipV = true;
}


//=============================================================================
// ヘルパ�E関数 / クラス
//=============================================================================

// レンダリング状慁ERTV, Viewport, Depth)をスコープ�Eで保存�E復允E��る�Eルパ�E
class ScopedRenderState 
{
public:

	ScopedRenderState(ID3D11DeviceContext* context) : m_Context(context)
	{
		m_Context->OMGetRenderTargets(1, &m_OldRTV, &m_OldDSV);
		m_Context->RSGetViewports(&m_NumViewports, &m_OldViewport);
		Renderer::SetDepthEnable(false); // ミニマップ描画中は深度無効
	}

	~ScopedRenderState() 
	{
		m_Context->OMSetRenderTargets(1, &m_OldRTV, m_OldDSV);
		m_Context->RSSetViewports(m_NumViewports, &m_OldViewport);
		if (m_OldRTV) m_OldRTV->Release();
		if (m_OldDSV) m_OldDSV->Release();
		Renderer::SetDepthEnable(true); // 深度有効に戻ぁE
	}

private:

	ID3D11DeviceContext* m_Context = nullptr;
	ID3D11RenderTargetView* m_OldRTV = nullptr;
	ID3D11DepthStencilView* m_OldDSV = nullptr;
	D3D11_VIEWPORT          m_OldViewport = {};
	UINT                    m_NumViewports = 1;
};



static Vector2 ApplyUVFlip(Vector2 uv, bool flipU, bool flipV)
{
	if (flipU) uv.x = 1.0f - uv.x;
	if (flipV) uv.y = 1.0f - uv.y;
	return uv;
}



static Vector2 WorldToMinimapUVRaw(const Vector3& pos)
{
	const float bs = MAP::Config::BLOCK_SIZE;
	const float half = bs * 0.5f;
	const float centerX = (MAP::Config::MaxX / 2.0f) * bs;
	const float centerZ = (MAP::Config::MaxY / 2.0f) * bs;

	const float worldRightX = half + centerX;
	const float worldLeftX = worldRightX - bs * (MAP::Config::MaxX - 1);
	const float worldTopZ = -half + centerZ;
	const float worldBottomZ = worldTopZ - bs * (MAP::Config::MaxY - 1);

	float u = (pos.x - worldLeftX) / (worldRightX - worldLeftX);
	float v = (pos.z - worldBottomZ) / (worldTopZ - worldBottomZ);

	return ApplyUVFlip({ u, v }, kFlipMiniMapU, kFlipMiniMapV);
}



// ワールド座標をミニマップUV(0�E�E)に変換し、篁E��外をクランプすめE
static Vector2 WorldToMinimapUV(const Vector3& pos)
{
	auto uv = WorldToMinimapUVRaw(pos);
	uv.x = std::clamp(uv.x, 0.0f, 1.0f);
	uv.y = std::clamp(uv.y, 0.0f, 1.0f);
	return uv;
}

//=============================================================================
// MiniMap クラス実裁E
//=============================================================================

MiniMap::MiniMap() {}

void MiniMap::Init()
{
	VisualObject::Init();

	//---------------------------------------------------------
	// リソース読み込み
	//---------------------------------------------------------
	m_TexWall.Load("assets/texture/icon_wall.png");
	m_TexGoal.Load("assets/texture/icon_goal.png");
	m_TexItem.Load("assets/texture/icon_item.png");
	m_TexPlayer.Load("assets/texture/icon_player.png");
	m_TexFogBrush.Load("assets/texture/fog_brush.png");
	m_TexFrame.Load("assets/texture/minimap_frame.png");

	m_MiniMapShader = std::make_unique<Shader>();
	m_MiniMapShader->Create("shader/MiniMapVS.hlsl", "shader/MiniMapPS.hlsl");
	//std::cout << "[MiniMap] Shader loaded successfully" << std::endl;

	//---------------------------------------------------------
	// 定数バッファ作�E (UV調整用)
	//---------------------------------------------------------
	D3D11_BUFFER_DESC cbDesc = {};
	cbDesc.ByteWidth = sizeof(UVAdjustBuffer);
	cbDesc.Usage = D3D11_USAGE_DEFAULT;
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	HRESULT hr = Renderer::GetDevice()->CreateBuffer(&cbDesc, nullptr, m_UVAdjustCB.GetAddressOf());
	if (SUCCEEDED(hr)) {
		std::cout << "[MiniMap] UV Adjust constant buffer created successfully" << std::endl;
	}
	else {
		std::cout << "[MiniMap] ERROR: Failed to create UV Adjust constant buffer" << std::endl;
	}

	CreateRenderTargets();

	CreateStaticMapResources();// 事前描画用リソース作�E

	// 初期化：探索マッチEFog)を黒でクリア
	float clearColorBlack[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	Renderer::GetDeviceContext()->ClearRenderTargetView(m_FogRTV.Get(), clearColorBlack);
}



void MiniMap::SetMap(MakeMap* map)
{
	m_MapData = map;

	BakeStaticMap();

	if (m_FogRTV)
	{
		float clearColorBlack[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		Renderer::GetDeviceContext()->ClearRenderTargetView(m_FogRTV.Get(), clearColorBlack);
	}
}




void MiniMap::CreateRenderTargets()
{
	auto device = Renderer::GetDevice();

	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = (UINT)kTexSize;
	desc.Height = (UINT)kTexSize;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;


	// Fog用 (探索状況E
	device->CreateTexture2D(&desc, nullptr, &m_FogTex);
	device->CreateRenderTargetView(m_FogTex.Get(), nullptr, &m_FogRTV);
	device->CreateShaderResourceView(m_FogTex.Get(), nullptr, &m_FogSRV);


	// Map用 (アイコンの雁E��)
	device->CreateTexture2D(&desc, nullptr, &m_MapTex);
	device->CreateRenderTargetView(m_MapTex.Get(), nullptr, &m_MapRTV);
	device->CreateShaderResourceView(m_MapTex.Get(), nullptr, &m_MapSRV);
}




void MiniMap::CreateStaticMapResources()
{
	auto device = Renderer::GetDevice();

	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = (UINT)kTexSize;
	desc.Height = (UINT)kTexSize;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

	device->CreateTexture2D(&desc, nullptr, &m_StaticMapTex);
	device->CreateRenderTargetView(m_StaticMapTex.Get(), nullptr, &m_StaticMapRTV);
	device->CreateShaderResourceView(m_StaticMapTex.Get(), nullptr, &m_StaticMapSRV);
}




// ---------------------------------------------------------
// 壁を事前描画する
// ---------------------------------------------------------
void MiniMap::BakeStaticMap()
{
	if (!m_MapData) return;

	auto context = Renderer::GetDeviceContext();
	ScopedRenderState renderState(context); // 状態保孁E

	SetupFlatMaterial();

	// 描画先を静的マップ用チE��スチャに設宁E
	context->OMSetRenderTargets(1, m_StaticMapRTV.GetAddressOf(), nullptr);

	// 透�Eでクリア
	float clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	context->ClearRenderTargetView(m_StaticMapRTV.Get(), clearColor);

	// ビューポ�Eト�E行�E設宁E
	D3D11_VIEWPORT vp = { 0.0f, 0.0f, (float)kTexSize, (float)kTexSize, 0.0f, 1.0f };
	context->RSSetViewports(1, &vp);

	Matrix view = Matrix::Identity;
	Matrix proj = Matrix::CreateOrthographicOffCenter(0, kTexSize, kTexSize, 0, 0, 1);
	Renderer::SetViewMatrix(&view);
	Renderer::SetProjectionMatrix(&proj);

	// 描画準備
	VisualObject::m_Shader.SetGPU();
	Renderer::SetBlendState(BS_ALPHABLEND); // 通常ブレンチE
	m_VertexBuffer.SetGPU();
	m_IndexBuffer.SetGPU();
	m_TexWall.SetGPU(); // 壁テクスチャセチE��

	const auto& mapArray = m_MapData->GetMap();

	auto GetDrawPosFromGrid = [&](int gridX, int gridY) -> Vector2
		{
			const float HALF_BLOCK = MAP::Config::BLOCK_SIZE / 2.0f;
			const float MAP_CENTER_X = MAP::Config::MaxX / 2.0f * MAP::Config::BLOCK_SIZE;
			const float MAP_CENTER_Y = MAP::Config::MaxY / 2.0f * MAP::Config::BLOCK_SIZE;
			float worldX = 0.0f + (HALF_BLOCK + MAP_CENTER_X - (MAP::Config::BLOCK_SIZE * gridX));
			float worldZ = 0.0f - (HALF_BLOCK - MAP_CENTER_Y + (MAP::Config::BLOCK_SIZE * gridY));
			auto uv = WorldToMinimapUV({ worldX, 0.0f, worldZ });
			return Vector2(uv.x * kTexSize, uv.y * kTexSize);
		};

	float cellScaleX = kTexSize / (float)MAP::Config::MaxX;
	float cellScaleY = kTexSize / (float)MAP::Config::MaxY;

	// 壁ルーチE
	for (int x = 0; x < MAP::Config::MaxX; ++x)
	{
		for (int y = 0; y < MAP::Config::MaxY; ++y)
		{
			if (mapArray[x][y] == 1)
			{
				Vector2 drawPos = GetDrawPosFromGrid(x, y);
				Matrix world = Matrix::CreateScale(cellScaleX, cellScaleY, 1.0f) *
					Matrix::CreateTranslation(drawPos.x, drawPos.y, 0.0f);
				Renderer::SetWorldMatrix(&world);
				context->DrawIndexed(6, 0, 0);
			}
		}
	}
}




void MiniMap::Uninit()
{

}


void MiniMap::Update(float deltaTime)
{
	// マップ拡大刁E��替ぁE
	if (Input::GetKeyPress('M') && !m_PrevMKey)
	{
		m_IsLargeMap = !m_IsLargeMap;
		//std::cout << "[MiniMap] Map mode: " << (m_IsLargeMap ? "LARGE" : "MINI") << std::endl;
	}

	m_PrevMKey = Input::GetKeyPress('M');

	// プレイヤー位置の更新
	Vector3 playerPos = Vector3::Zero;
	if (auto player = SceneManager::GetInstance().FindObject<Player>().lock())
	{
		playerPos = player->GetPosition();
		auto uv = WorldToMinimapUV(playerPos);

		m_PlayerTexX = uv.x;
		m_PlayerTexY = uv.y;

#ifdef _DEBUG
		static int dbgCnt = 0;
		if (++dbgCnt % 60 == 0)
		{
			// UV変換の中間値も表示して検証
			const float bs = MAP::Config::BLOCK_SIZE;
			const float half = bs * 0.5f;
			const float cX = (MAP::Config::MaxX / 2.0f) * bs;
			const float cZ = (MAP::Config::MaxY / 2.0f) * bs;
			const float wR = half + cX;
			const float wL = wR - bs * (MAP::Config::MaxX - 1);
			const float wT = -half + cZ;
			const float wB = wT - bs * (MAP::Config::MaxY - 1);
			float rawU = (playerPos.x - wL) / (wR - wL);
			float rawV = (playerPos.z - wB) / (wT - wB);
			std::cout << "[MiniMap] World=(" << playerPos.x << "," << playerPos.z
				<< ") rawUV=(" << rawU << "," << rawV
				<< ") finalUV=(" << uv.x << "," << uv.y << ")" << std::endl;
		}
#endif
	}
	else
	{
#ifdef _DEBUG
		static bool warnedMinimap = false;
		if (!warnedMinimap)
		{
			std::cout << "[MiniMap ERROR] FindObject<Player> FAILED!" << std::endl;
			warnedMinimap = true;
		}
#endif
	}

	UpdateFog(playerPos);
	DrawMapIcons();
}


void MiniMap::Draw()
{
	DrawUI();
}


//=============================================================================
// レンダリング処琁E
//=============================================================================

void MiniMap::DrawMapIcons()
{
	if (!m_MapData) return;

	auto context = Renderer::GetDeviceContext();

	// スチE�Eト保孁E
	ScopedRenderState renderState(context);
	SetupFlatMaterial();

	// 描画先をマップ用チE��スチャに刁E��替ぁE
	context->OMSetRenderTargets(1, m_MapRTV.GetAddressOf(), nullptr);

	// 背景を半透�Eの黒でクリア
	float clearColor[] = { 0.0f, 0.0f, 0.0f, 0.5f };
	context->ClearRenderTargetView(m_MapRTV.Get(), clearColor);

	// ビューポ�Eト設宁E
	D3D11_VIEWPORT vp = { 0.0f, 0.0f, (float)kTexSize, (float)kTexSize, 0.0f, 1.0f };
	context->RSSetViewports(1, &vp);

	// 2D投影行�Eの設宁E
	Matrix view = Matrix::Identity;
	Matrix proj = Matrix::CreateOrthographicOffCenter(0, kTexSize, kTexSize, 0, 0, 1);
	Renderer::SetViewMatrix(&view);
	Renderer::SetProjectionMatrix(&proj);


	// 共通スチE�Eト設宁E
	VisualObject::m_Shader.SetGPU();
	Renderer::SetBlendState(BS_ALPHABLEND);
	m_VertexBuffer.SetGPU();
	m_IndexBuffer.SetGPU();


	// -------------------------------------------------------
	// 事前生�Eした静的マッチE壁Eを描画
	// -------------------------------------------------------
	{
		// チE��スチャ全体に引き伸ばして描画
		Matrix world = Matrix::CreateScale((float)kTexSize, (float)kTexSize, 1.0f) *
			Matrix::CreateTranslation((float)kTexSize * 0.5f, (float)kTexSize * 0.5f, 0.0f);

		Renderer::SetWorldMatrix(&world);

		// BakeしたチE��スチャを使用
		context->PSSetShaderResources(0, 1, m_StaticMapSRV.GetAddressOf());

		context->DrawIndexed(6, 0, 0);
	}


	// -------------------------------------------------------
	// 動的オブジェクチEゴール筁Eの描画
	// -------------------------------------------------------
	int sx, sy, gx, gy;
	m_MapData->GetStartGoal(sx, sy, gx, gy);

	// グリチE��サイズ計箁E
	float cellScaleX = kTexSize / (float)MAP::Config::MaxX;
	float cellScaleY = kTexSize / (float)MAP::Config::MaxY;

	// ゴール座標計箁E
	const float HALF_BLOCK = MAP::Config::BLOCK_SIZE / 2.0f;
	const float MAP_CENTER_X = MAP::Config::MaxX / 2.0f * MAP::Config::BLOCK_SIZE;
	const float MAP_CENTER_Y = MAP::Config::MaxY / 2.0f * MAP::Config::BLOCK_SIZE;

	float worldX = 0.0f + (HALF_BLOCK + MAP_CENTER_X - (MAP::Config::BLOCK_SIZE * gx));
	float worldZ = 0.0f - (HALF_BLOCK - MAP_CENTER_Y + (MAP::Config::BLOCK_SIZE * gy));

	auto uv = WorldToMinimapUV({ worldX, 0.0f, worldZ });
	Vector2 drawPos(uv.x * kTexSize, uv.y * kTexSize);

	// 描画実衁E
	Matrix world = Matrix::CreateScale(cellScaleX, cellScaleY, 1.0f) *
		Matrix::CreateTranslation(drawPos.x, drawPos.y, 0.0f);
	Renderer::SetWorldMatrix(&world);

	m_TexGoal.SetGPU();
	context->DrawIndexed(6, 0, 0);
}





void MiniMap::UpdateFog(const Vector3& playerPos)
{
	auto context = Renderer::GetDeviceContext();
	ScopedRenderState renderState(context); // 状態�E保存と復允E

	SetupFlatMaterial();

	// 描画先をFogチE��スチャに
	context->OMSetRenderTargets(1, m_FogRTV.GetAddressOf(), nullptr);

	D3D11_VIEWPORT vp = { 0.0f, 0.0f, (float)kTexSize, (float)kTexSize, 0.0f, 1.0f };
	context->RSSetViewports(1, &vp);

	Matrix view = Matrix::Identity;
	Matrix proj = Matrix::CreateOrthographicOffCenter(0, kTexSize, kTexSize, 0, 0, 1);
	Renderer::SetViewMatrix(&view);
	Renderer::SetProjectionMatrix(&proj);


	// 描画位置計箁E
	auto uv = WorldToMinimapUV(playerPos);
	float drawX = uv.x * kTexSize;
	float drawY = uv.y * kTexSize;


	// 加算合成でブラシを描画
	Renderer::SetBlendState(BS_ADDITIVE);

	Matrix world = Matrix::CreateScale((float)m_FogBrushSize, (float)m_FogBrushSize, 1.0f) *

		Matrix::CreateTranslation(drawX, drawY, 0.0f);

	Renderer::SetWorldMatrix(&world);

	VisualObject::m_Shader.SetGPU();
	m_TexFogBrush.SetGPU();
	m_VertexBuffer.SetGPU();
	m_IndexBuffer.SetGPU();
	context->DrawIndexed(6, 0, 0);

	Renderer::SetBlendState(BS_ALPHABLEND);
}



void MiniMap::DrawUI()
{
	auto context = Renderer::GetDeviceContext();

	// 画面全体へのViewport設宁E
	D3D11_VIEWPORT screenVp = {};
	screenVp.Width = (float)Application::GetWidth();
	screenVp.Height = (float)Application::GetHeight();
	screenVp.MinDepth = 0.0f;
	screenVp.MaxDepth = 1.0f;
	context->RSSetViewports(1, &screenVp);

	Renderer::SetDepthEnable(false);
	Renderer::SetWorldViewProjection2D();

	SetupFlatMaterial();

	//---------------------------------------------------------
	// レイアウト計箁E(画面中央原点用)
	//---------------------------------------------------------
	const float screenW = (float)Application::GetWidth();
	const float screenH = (float)Application::GetHeight();
	const float halfW = screenW * 0.5f;
	const float halfH = screenH * 0.5f;

	// マップ表示サイズ
	float mapSize = m_IsLargeMap ? (screenH * kLargeMapScreenRate) : kSmallMapSize;

	// フレームサイズ計箁E(アスペクト比維持E
	float frameTexW = (float)m_TexFrame.GetWidth();
	float frameTexH = (float)m_TexFrame.GetHeight();
	float frameAspect = (frameTexH != 0.0f) ? (frameTexW / frameTexH) : 1.0f;

	float frameHeight = mapSize / kFrameInnerRate;
	float frameWidth = frameHeight * frameAspect;

	float worldX = 0.0f;
	float worldY = 0.0f;

	if (m_IsLargeMap)
	{
		// ど真ん中
		worldX = 0.0f;
		worldY = 0.0f;
	}
	else
	{
		// 右上�E置
		// X: 中忁E��ら右へ (+) -> (画面半�E - マ�Eジン - フレーム半征E
		worldX = halfW - kMapMargin - (frameWidth * 0.5f);

		// Y: 中忁E��ら上へ (+) -> (画面半�E - マ�Eジン - フレーム半征E
		worldY = halfH - kMapMargin - (frameHeight * 0.5f);
	}

	// 表示上�E中忁E��標（アイコン描画用オフセチE��計算などに使用�E�E
	float centerX = worldX + halfW; // スクリーン座標系への換算が忁E��な場合用(基本使わなぁE

	//---------------------------------------------------------
	// マップ本体�E描画 (MapSRV + FogSRV)
	//---------------------------------------------------------
	Vector2 usedUvOffset(0.0f, 0.0f);
	Vector2 usedUvScale(1.0f, 1.0f);
	{
		Matrix world = Matrix::CreateScale(mapSize, mapSize, 1.0f) *
			Matrix::CreateTranslation(worldX, worldY, 0.5f);

		Renderer::SetWorldMatrix(&world);

		m_MiniMapShader->SetGPU();

		// UVパラメータ計箁E
		UVAdjustBuffer uvAdjust;
		if (m_IsLargeMap)
		{
			uvAdjust.uvScale = Vector2(1.0f, 1.0f);
			uvAdjust.uvOffset = Vector2(0.0f, 0.0f);
		}
		else
		{
			uvAdjust.uvScale = Vector2(kZoomLevel, kZoomLevel);

			// プレイヤーを中忁E��持ってくるためのオフセチE��
			uvAdjust.uvOffset = Vector2(
				m_PlayerTexX - kZoomLevel * 0.5f,
				m_PlayerTexY - kZoomLevel * 0.5f
			);

			// 篁E��外に行かなぁE��ぁE��クランチE
			if (uvAdjust.uvOffset.x < 0.0f) uvAdjust.uvOffset.x = 0.0f;
			if (uvAdjust.uvOffset.y < 0.0f) uvAdjust.uvOffset.y = 0.0f;
			if (uvAdjust.uvOffset.x + kZoomLevel > 1.0f) uvAdjust.uvOffset.x = 1.0f - kZoomLevel;
			if (uvAdjust.uvOffset.y + kZoomLevel > 1.0f) uvAdjust.uvOffset.y = 1.0f - kZoomLevel;
		}

		usedUvOffset = uvAdjust.uvOffset;
		usedUvScale = uvAdjust.uvScale;

		context->UpdateSubresource(m_UVAdjustCB.Get(), 0, nullptr, &uvAdjust, 0, 0);
		context->PSSetConstantBuffers(5, 1, m_UVAdjustCB.GetAddressOf());

		ID3D11ShaderResourceView* srvs[] = { m_MapSRV.Get(), m_FogSRV.Get() };
		context->PSSetShaderResources(0, 2, srvs);

		Renderer::SetBlendState(BS_ALPHABLEND);
		m_VertexBuffer.SetGPU();
		m_IndexBuffer.SetGPU();
		context->DrawIndexed(6, 0, 0);
	}

	//---------------------------------------------------------
	// フレームの描画
	//---------------------------------------------------------
	if (!m_IsLargeMap)
	{
		VisualObject::m_Shader.SetGPU();
		Matrix world = Matrix::CreateScale(frameWidth, frameHeight, 1.0f) *
			Matrix::CreateTranslation(worldX, worldY, 0.5f);
		Renderer::SetWorldMatrix(&world);

		m_TexFrame.SetGPU();
		context->DrawIndexed(6, 0, 0);
	}

	//---------------------------------------------------------
	// プレイヤーアイコンの描画
	//---------------------------------------------------------
	{
		VisualObject::m_Shader.SetGPU();

		// アイコンのワールド座標（中忁E��ら�E相対座標）を計箁E
		float iconWorldX = 0.0f;
		float iconWorldY = 0.0f;

		if (m_IsLargeMap)
		{
			// テクスチャUV → クワッドスクリーン座標
			iconWorldX = (m_PlayerTexX - 0.5f) * mapSize;
			iconWorldY = (m_PlayerTexY - 0.5f) * mapSize;
		}
		else
		{
			// 小マップ: シェーダーのUV変換に合わせてローカルUVを計算
			// シェーダー: adjustedUV = input.tex * uvScale + uvOffset
			// input.tex に対応するローカルUV
			float localU = (m_PlayerTexX - usedUvOffset.x) / usedUvScale.x;
			float localV = (m_PlayerTexY - usedUvOffset.y) / usedUvScale.y;

			localU = std::clamp(localU, 0.0f, 1.0f);
			localV = std::clamp(localV, 0.0f, 1.0f);

			// テクスチャUV → クワッドスクリーン座標
			iconWorldX = worldX + (localU - 0.5f) * mapSize;
			iconWorldY = worldY + (localV - 0.5f) * mapSize;
		}

		// 回転計箁E
		float rot = 0.0f;
		if (auto p = SceneManager::GetInstance().FindObject<Player>().lock())
		{
			Vector3 pos = p->GetPosition();
			Vector3 f = p->GetForward();

			f.y = 0.0f;
			if (f.LengthSquared() < 1e-6f) f = Vector3::UnitZ;
			f.Normalize();

			auto uv0 = WorldToMinimapUVRaw(pos);
			auto uv1 = WorldToMinimapUVRaw(pos + f * 10.0f);

			Vector2 q0 = ApplyUVFlip((uv0 - usedUvOffset) / usedUvScale, kQuadFlipU, kQuadFlipV);
			Vector2 q1 = ApplyUVFlip((uv1 - usedUvOffset) / usedUvScale, kQuadFlipU, kQuadFlipV);

			Vector2 d = q1 - q0;

			rot = std::atan2(d.y, d.x) - XM_PIDIV2;
		}

		Matrix world = Matrix::CreateScale(kPlayerIconSize, kPlayerIconSize, 1.0f) *
			Matrix::CreateRotationZ(rot) *
			Matrix::CreateTranslation(iconWorldX, iconWorldY, 0.5f);

		Renderer::SetWorldMatrix(&world);

		m_TexPlayer.SetGPU();
		context->DrawIndexed(6, 0, 0);
	}

	// シェーダーリソース解除
	ID3D11ShaderResourceView* nullSRVs[] = { nullptr, nullptr };
	context->PSSetShaderResources(0, 2, nullSRVs);
	Renderer::SetDepthEnable(true);
}