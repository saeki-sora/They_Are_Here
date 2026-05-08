#include "pch.h"
#include "MiniMap.h"
#include "SceneManager.h"
#include "Input.h"
#include "Renderer.h"
#include "MakeMap.h"
#include "Player.h"
#include "GoalKey.h"
#include "Pole.h"
#include "Application.h"


using namespace DirectX;
using namespace DirectX::SimpleMath;



static void SetupFlatMaterial()
{
	MATERIAL mat = {};
	mat.Ambient = DirectX::SimpleMath::Color(1.0f, 1.0f, 1.0f, 1.0f);// 環境光MAX
	mat.Emission = DirectX::SimpleMath::Color(1.0f, 1.0f, 1.0f, 1.0f); // 発光MAX
	mat.Diffuse = DirectX::SimpleMath::Color(0.0f, 0.0f, 0.0f, 0.0f); // 拡散光なし
	mat.TextureEnable = TRUE;
	Renderer::SetMaterial(mat);
}

//=============================================================================
// 定数・設定
//=============================================================================
namespace {

	// ミニマップテクスチャサイズ
	constexpr int   kTexSize = 512;

	// UI表示設定
	constexpr float kSmallMapSize = 250.0f;
	constexpr float kLargeMapScreenRate = 0.8f;// 拡大時の画面占有率.0-1.0の範囲
	constexpr float kFrameInnerRate = 0.7f;
	constexpr float kMapMargin = 20.0f;// 画面端からのマップの余白
	constexpr float kZoomLevel = 0.3f; // 拡大時の表示範囲.0-1.0の範囲

	// アイコンサイズ
	constexpr float kPlayerIconSize = 32.0f;

	// 方向インジケーター設定
	constexpr float kDirArrowSize = 40.0f;  // 矢印テクスチャの描画サイズ
	constexpr float kDirIconSize = 22.0f;  // アイコンテクスチャの描画サイズ
	constexpr float kDirEdgeRate = 0.47f;  // ミニマップ縁への寄せ率（0.5=縁ぴったり）

	// UV反転設定
	constexpr bool  kFlipMiniMapU = false;
	constexpr bool  kFlipMiniMapV = false;
	constexpr bool  kQuadFlipU = false;
	constexpr bool  kQuadFlipV = false;
}


//=============================================================================
// ヘルパー関数 / クラス
//=============================================================================
// レンダリング状態(ERTV, Viewport, Depth)をスコープで保存・復元するヘルパー
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
		Renderer::SetDepthEnable(true);// 復元後は深度有効に戻す
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



// mapData でマップのセル数を取得することで、難易度が変わってもミニマップ座標が正しく計算される
static Vector2 WorldToMinimapUVRaw(const MakeMap* mapData, const Vector3& pos)
{
	const float bs = MAP::Config::BLOCK_SIZE;
	const float half = bs * 0.5f;
	const float centerX = (mapData->GetSizeX() / 2.0f) * bs;
	const float centerZ = (mapData->GetSizeY() / 2.0f) * bs;

	// MakeMap.cpp の座標系
	const float xMax = centerX + half;                              // 画面右端（X最大）
	const float xMin = centerX + half - bs * mapData->GetSizeX();  // 画面左端（X最小）
	const float zMax = centerZ - half;                              // 画面上端（Z最大）
	const float zMin = centerZ - half - bs * mapData->GetSizeY();  // 画面下端（Z最小）

	// U: 右が0, 左が1
	// V: 上が0, 下が1
	float u = 1.0f - (pos.x - xMin) / (xMax - xMin);
	float v = 1.0f - (pos.z - zMin) / (zMax - zMin);

	return ApplyUVFlip({ u, v }, kFlipMiniMapU, kFlipMiniMapV);
}



// ワールド座標をミニマップUV(0-1)に変換し、範囲外をクランプする
static Vector2 WorldToMinimapUV(const MakeMap* mapData, const Vector3& pos)
{
	auto uv = WorldToMinimapUVRaw(mapData, pos);
	uv.x = std::clamp(uv.x, 0.0f, 1.0f);
	uv.y = std::clamp(uv.y, 0.0f, 1.0f);
	return uv;
}

//============================================================
// MiniMap クラス実装
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

	m_TexDirArrow.Load("assets/texture/icon_direction_arrow.png");
	m_TexDirKey.Load("assets/texture/icon_direction_key.png");
	m_TexDirGoal.Load("assets/texture/icon_direction_goal.png");

	m_MiniMapShader = std::make_unique<Shader>();
	m_MiniMapShader->Create("shader/MiniMapVS.hlsl", "shader/MiniMapPS.hlsl");
	//std::cout << "[MiniMap] Shader loaded successfully" << std::endl;

	//---------------------------------------------------------
	// 定数バッファ作成 (UV調整用)
	//---------------------------------------------------------
	D3D11_BUFFER_DESC cbDesc = {};
	cbDesc.ByteWidth = sizeof(UVAdjustBuffer);
	cbDesc.Usage = D3D11_USAGE_DEFAULT;
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	HRESULT hr = Renderer::GetDevice()->CreateBuffer(&cbDesc, nullptr, m_UVAdjustCB.GetAddressOf());
	if (SUCCEEDED(hr))
	{
		//std::cout << "[MiniMap] UV Adjust constant buffer created successfully" << std::endl;
	}
	else
	{
		//std::cout << "[MiniMap] ERROR: Failed to create UV Adjust constant buffer" << std::endl;
	}

	CreateRenderTargets();

	CreateStaticMapResources();// 事前描画用リソース作成

	// 初期化：探索マップ用(Fog)を黒でクリア
	float clearColorBlack[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	Renderer::GetDeviceContext()->ClearRenderTargetView(m_FogRTV.Get(), clearColorBlack);
}



// マップデータをセットする関数
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


	// Fog用 (探索状況用)
	device->CreateTexture2D(&desc, nullptr, &m_FogTex);
	device->CreateRenderTargetView(m_FogTex.Get(), nullptr, &m_FogRTV);
	device->CreateShaderResourceView(m_FogTex.Get(), nullptr, &m_FogSRV);


	// Map用 (アイコンの描画用)
	device->CreateTexture2D(&desc, nullptr, &m_MapTex);
	device->CreateRenderTargetView(m_MapTex.Get(), nullptr, &m_MapRTV);
	device->CreateShaderResourceView(m_MapTex.Get(), nullptr, &m_MapSRV);
}



// 事前描画用のリソースを作成する関数
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
	ScopedRenderState renderState(context); // 状態保管

	SetupFlatMaterial();

	// 描画先を静的マップ用テクスチャに設定
	context->OMSetRenderTargets(1, m_StaticMapRTV.GetAddressOf(), nullptr);

	// 透過でクリア
	float clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	context->ClearRenderTargetView(m_StaticMapRTV.Get(), clearColor);

	// ビューポート行設定
	D3D11_VIEWPORT vp = { 0.0f, 0.0f, (float)kTexSize, (float)kTexSize, 0.0f, 1.0f };
	context->RSSetViewports(1, &vp);

	Matrix view = Matrix::Identity;
	Matrix proj = Matrix::CreateOrthographicOffCenter(0, kTexSize, kTexSize, 0, 0, 1);
	Renderer::SetViewMatrix(&view);
	Renderer::SetProjectionMatrix(&proj);

	// 描画準備
	VisualObject::m_Shader.SetGPU();
	Renderer::SetBlendState(BS_ALPHABLEND); // 通常ブレンディング
	m_VertexBuffer.SetGPU();
	m_IndexBuffer.SetGPU();
	m_TexWall.SetGPU(); // 壁テクスチャ設定

	const auto& mapArray = m_MapData->GetMap();

	auto GetDrawPosFromGrid = [&](int gridX, int gridY) -> Vector2
		{
			const float HALF_BLOCK = MAP::Config::BLOCK_SIZE / 2.0f;
			const float MAP_CENTER_X = m_MapData->GetSizeX() / 2.0f * MAP::Config::BLOCK_SIZE;
			const float MAP_CENTER_Y = m_MapData->GetSizeY() / 2.0f * MAP::Config::BLOCK_SIZE;

			float worldX = HALF_BLOCK + MAP_CENTER_X - MAP::Config::BLOCK_SIZE * gridX;
			float worldZ = -(HALF_BLOCK - MAP_CENTER_Y + MAP::Config::BLOCK_SIZE * gridY);

			auto uv = WorldToMinimapUV(m_MapData, { worldX, 0.0f, worldZ });

			return Vector2(uv.x * kTexSize, uv.y * kTexSize);
		};

	float cellScaleX = kTexSize / (float)m_MapData->GetSizeX();
	float cellScaleY = kTexSize / (float)m_MapData->GetSizeY();

	// 壁ルーチン
	for (int x = 0; x < m_MapData->GetSizeX(); ++x)
	{
		for (int y = 0; y < m_MapData->GetSizeY(); ++y)
		{
			// 値が0（通路）以外のすべての障害物（壁1、柱2、建設中3など）を描画する
			if (mapArray[x][y] != 0)
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
	// マップ拡大縮小
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
		auto uv = WorldToMinimapUV(m_MapData, playerPos);

		m_PlayerTexX = uv.x;
		m_PlayerTexY = uv.y;

#ifdef _DEBUG
		static int dbgCnt = 0;
		if (++dbgCnt % 60 == 0)
		{
			// UV変換の中間値も表示して検証
			const float bs = MAP::Config::BLOCK_SIZE;
			const float half = bs * 0.5f;
			const float cX = (m_MapData->GetSizeX() / 2.0f) * bs;
			const float cZ = (m_MapData->GetSizeY() / 2.0f) * bs;
			const float wR = half + cX;
			const float wL = wR - bs * (m_MapData->GetSizeX() - 1);
			const float wT = -half + cZ;
			const float wB = wT - bs * (m_MapData->GetSizeY() - 1);
			float rawU = (playerPos.x - wL) / (wR - wL);
			float rawV = (playerPos.z - wB) / (wT - wB);
			// std::cout << "[MiniMap] World=(" << playerPos.x << "," << playerPos.z
			// 	<< ") rawUV=(" << rawU << "," << rawV
			// 	<< ") finalUV=(" << uv.x << "," << uv.y << ")" << std::endl;
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

	if (!m_MapData) return; // マップがセットされていなければ描画しない
	UpdateFog(playerPos);
	DrawMapIcons();
}


void MiniMap::Draw()
{
	DrawUI();
}


//=============================================================================
// レンダリング処理
//=============================================================================
void MiniMap::DrawMapIcons()
{
	if (!m_MapData) return;

	auto context = Renderer::GetDeviceContext();

	// スチルテクスチャ保管
	ScopedRenderState renderState(context);
	SetupFlatMaterial();

	// 描画先をマップ用テクスチャに設定
	context->OMSetRenderTargets(1, m_MapRTV.GetAddressOf(), nullptr);

	// 背景を半透明の黒でクリア
	float clearColor[] = { 0.0f, 0.0f, 0.0f, 0.5f };
	context->ClearRenderTargetView(m_MapRTV.Get(), clearColor);

	// ビューポート設定
	D3D11_VIEWPORT vp = { 0.0f, 0.0f, (float)kTexSize, (float)kTexSize, 0.0f, 1.0f };
	context->RSSetViewports(1, &vp);

	// 2D投影行列の設定
	Matrix view = Matrix::Identity;
	Matrix proj = Matrix::CreateOrthographicOffCenter(0, kTexSize, kTexSize, 0, 0, 1);
	Renderer::SetViewMatrix(&view);
	Renderer::SetProjectionMatrix(&proj);


	// 共通スチルテクスチャ設定
	VisualObject::m_Shader.SetGPU();
	Renderer::SetBlendState(BS_ALPHABLEND);
	m_VertexBuffer.SetGPU();
	m_IndexBuffer.SetGPU();


	// -------------------------------------------------------
	// 事前生成した静的マップ用壁を描画
	// -------------------------------------------------------
	{
		// 壁の描画
		Matrix world = Matrix::CreateScale((float)kTexSize, -(float)kTexSize, 1.0f) *
			Matrix::CreateTranslation((float)kTexSize * 0.5f, (float)kTexSize * 0.5f, 0.0f);

		Renderer::SetWorldMatrix(&world);

		// 事前生成した壁テクスチャを描画
		context->PSSetShaderResources(0, 1, m_StaticMapSRV.GetAddressOf());

		context->DrawIndexed(6, 0, 0);
	}


	// -------------------------------------------------------
	// 動的オブジェクチゴールの描画
	// -------------------------------------------------------
	int sx, sy, gx, gy;
	m_MapData->GetStartGoal(sx, sy, gx, gy);

	// グリッドサイズ計算
	float cellScaleX = kTexSize / (float)m_MapData->GetSizeX();
	float cellScaleY = kTexSize / (float)m_MapData->GetSizeY();

	// ゴール座標計算
	const float HALF_BLOCK = MAP::Config::BLOCK_SIZE / 2.0f;
	const float MAP_CENTER_X = m_MapData->GetSizeX() / 2.0f * MAP::Config::BLOCK_SIZE;
	const float MAP_CENTER_Y = m_MapData->GetSizeY() / 2.0f * MAP::Config::BLOCK_SIZE;

	float worldX = HALF_BLOCK + MAP_CENTER_X - MAP::Config::BLOCK_SIZE * gx;
	float worldZ = -(HALF_BLOCK - MAP_CENTER_Y + MAP::Config::BLOCK_SIZE * gy);

	// ワールド座標をミニマップUVに変換
	auto uv = WorldToMinimapUV(m_MapData, { worldX, 0.0f, worldZ });
	Vector2 drawPos(uv.x * kTexSize, uv.y * kTexSize);

	// 描画実行
	Matrix world = Matrix::CreateScale(cellScaleX, cellScaleY, 1.0f) *
		Matrix::CreateTranslation(drawPos.x, drawPos.y, 0.0f);
	Renderer::SetWorldMatrix(&world);

	m_TexGoal.SetGPU();
	context->DrawIndexed(6, 0, 0);
}





void MiniMap::UpdateFog(const Vector3& playerPos)
{
	auto context = Renderer::GetDeviceContext();
	ScopedRenderState renderState(context); // 状態保管

	SetupFlatMaterial();

	// 描画先をFogテクスチャに
	context->OMSetRenderTargets(1, m_FogRTV.GetAddressOf(), nullptr);

	D3D11_VIEWPORT vp = { 0.0f, 0.0f, (float)kTexSize, (float)kTexSize, 0.0f, 1.0f };
	context->RSSetViewports(1, &vp);

	Matrix view = Matrix::Identity;
	Matrix proj = Matrix::CreateOrthographicOffCenter(0, kTexSize, kTexSize, 0, 0, 1);
	Renderer::SetViewMatrix(&view);
	Renderer::SetProjectionMatrix(&proj);


	// 描画位置計算
	auto uv = WorldToMinimapUV(m_MapData, playerPos);
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

	// 画面全体へのViewport設定
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
	// レイアウト計算(画面中央原点用)
	//---------------------------------------------------------
	const float screenW = (float)Application::GetWidth();
	const float screenH = (float)Application::GetHeight();
	const float halfW = screenW * 0.5f;
	const float halfH = screenH * 0.5f;

	// マップ表示サイズ
	float mapSize = m_IsLargeMap ? (screenH * kLargeMapScreenRate) : kSmallMapSize;

	// フレームサイズ計算(アスペクト比維持)
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
		// 右上配置
		// X: 中心から右へ (+) -> (画面半分 - マージン - フレーム半分)
		worldX = halfW - kMapMargin - (frameWidth * 0.5f);

		// Y: 中心から上へ (+) -> (画面半分 - マージン - フレーム半分)
		worldY = halfH - kMapMargin - (frameHeight * 0.5f);
	}

	// 表示上の中心座標（アイコン描画用オフセット計算などに使用）
	float centerX = worldX + halfW; // スクリーン座標系への換算が必要な場合用(基本使わない)

	//---------------------------------------------------------
	// UVパラメータの設定
	//---------------------------------------------------------
	Vector2 usedUvOffset(0.0f, 0.0f);
	Vector2 usedUvScale(1.0f, 1.0f);
	float mapRotAngle = 0.0f;
	{
		Matrix world = Matrix::CreateScale(mapSize, mapSize, 1.0f) *
			Matrix::CreateTranslation(worldX, worldY, 0.5f);

		Renderer::SetWorldMatrix(&world);

		m_MiniMapShader->SetGPU();

		// マップ回転角の計算（小マップのみ、プレイヤーの向きを上に固定）
		if (!m_IsLargeMap)
		{
			if (auto p = SceneManager::GetInstance().FindObject<Player>().lock())
			{
				Vector3 pos = p->GetPosition();
				Vector3 f = p->GetForward();
				f.y = 0.0f;
				if (f.LengthSquared() < 1e-6f) f = Vector3::UnitZ;
				f.Normalize();

				auto uv0 = WorldToMinimapUVRaw(m_MapData, pos);
				auto uv1 = WorldToMinimapUVRaw(m_MapData, pos + f * 10.0f);
				Vector2 d = uv1 - uv0;

				float iconRot = std::atan2(-d.x, -d.y);
				mapRotAngle = iconRot;
			}
		}

		// UVパラメータ計算
		UVAdjustBuffer uvAdjust;
		if (m_IsLargeMap)
		{
			uvAdjust.uvScale = Vector2(1.0f, 1.0f);
			uvAdjust.uvOffset = Vector2(0.0f, 0.0f);
			uvAdjust.mapRotation = 0.0f;
		}
		else
		{
			uvAdjust.uvScale = Vector2(kZoomLevel, kZoomLevel);

			// プレイヤーを中心に持ってくるためのオフセット
			uvAdjust.uvOffset = Vector2(
				m_PlayerTexX - kZoomLevel * 0.5f,
				m_PlayerTexY - kZoomLevel * 0.5f
			);

			uvAdjust.mapRotation = mapRotAngle;
		}
		uvAdjust._pad[0] = uvAdjust._pad[1] = uvAdjust._pad[2] = 0.0f;

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

		// アイコンのワールド座標（中心からの相対座標）を計算
		float iconWorldX = 0.0f;
		float iconWorldY = 0.0f;

		if (m_IsLargeMap)
		{
			// 大マップ: UV変換なしでテクスチャ座標を直接ワールド座標に変換
			iconWorldX = (m_PlayerTexX - 0.5f) * mapSize;
			iconWorldY = (0.5f - m_PlayerTexY) * mapSize; // V大→スクリーン下なので符号反転
		}
		else
		{
			// 小マップ: UVオフセットとスケールを考慮してテクスチャ座標をローカルUVに変換し、そこからワールド座標に変換
			float localU = (m_PlayerTexX - usedUvOffset.x) / usedUvScale.x;
			float localV = (m_PlayerTexY - usedUvOffset.y) / usedUvScale.y;

			localU = std::clamp(localU, 0.0f, 1.0f);
			localV = std::clamp(localV, 0.0f, 1.0f);

			// テクスチャUV → クワッドスクリーン座標
			// DrawUIはXMMatrixOrthographicLH（Y上正）を使用。
			// UV空間はY下正なので、V値の符号を反転する必要がある。
			iconWorldX = worldX + (localU - 0.5f) * mapSize;
			iconWorldY = worldY + (0.5f - localV) * mapSize; // V大→スクリーン下なので符号反転
		}

		// 回転計算（大マップのみ向き表示、小マップはマップ自体が回転するのでアイコンは上向き固定）
		float rot = 0.0f;
		if (m_IsLargeMap)
		{
			if (auto p = SceneManager::GetInstance().FindObject<Player>().lock())
			{
				Vector3 pos = p->GetPosition();
				Vector3 f = p->GetForward();

				f.y = 0.0f;
				if (f.LengthSquared() < 1e-6f) f = Vector3::UnitZ;
				f.Normalize();

				auto uv0 = WorldToMinimapUVRaw(m_MapData, pos);
				auto uv1 = WorldToMinimapUVRaw(m_MapData, pos + f * 10.0f);

				Vector2 q0 = ApplyUVFlip((uv0 - usedUvOffset) / usedUvScale, kQuadFlipU, kQuadFlipV);
				Vector2 q1 = ApplyUVFlip((uv1 - usedUvOffset) / usedUvScale, kQuadFlipU, kQuadFlipV);

				Vector2 d = q1 - q0;

				rot = std::atan2(-d.x, -d.y);
			}
		}

		Matrix world = Matrix::CreateScale(kPlayerIconSize, kPlayerIconSize, 1.0f) *
			Matrix::CreateRotationZ(rot) *
			Matrix::CreateTranslation(iconWorldX, iconWorldY, 0.5f);

		Renderer::SetWorldMatrix(&world);

		m_TexPlayer.SetGPU();
		context->DrawIndexed(6, 0, 0);
	}

	//---------------------------------------------------------
	// 方向インジケーター（小マップのみ）
	//---------------------------------------------------------
	if (!m_IsLargeMap)
	{
		DrawDirectionIndicator(worldX, worldY, mapSize, usedUvOffset, usedUvScale.x, mapRotAngle);
	}

	// シェーダーリソース解除
	ID3D11ShaderResourceView* nullSRVs[] = { nullptr, nullptr };
	context->PSSetShaderResources(0, 2, nullSRVs);
	Renderer::SetDepthEnable(true);
}



void MiniMap::DrawDirectionIndicator(
	float mapCX, float mapCY, float mapSize,
	const Vector2& uvOffset, float uvScale, float mapRotAngle)
{
	// 1. 対象位置を決定（鍵未取得→鍵、取得後→ゴール）
	Vector3 targetWorldPos;
	auto key = m_GoalKey.lock();
	bool showGoal = (!key || key->IsObtained());

	if (!showGoal)
		targetWorldPos = key->GetPosition();
	else
	{
		auto pole = m_GoalPole.lock();
		if (!pole) return;
		targetWorldPos = pole->GetPosition();
	}

	// 2. プレイヤー情報取得
	auto playerObj = SceneManager::GetInstance().FindObject<Player>().lock();
	if (!playerObj) return;
	Vector3 playerPos = playerObj->GetPosition();
	Vector3 playerFwd = playerObj->GetForward();
	playerFwd.y = 0.0f;
	if (playerFwd.LengthSquared() < 1e-6f) playerFwd = Vector3::UnitZ;
	playerFwd.Normalize();

	// 3. ターゲット方向（ワールド空間・水平）
	Vector3 toTarget = targetWorldPos - playerPos;
	toTarget.y = 0.0f;
	float dist = toTarget.Length();
	if (dist < 0.001f) return;
	toTarget /= dist;

	// 4. 可視チェック：ターゲットUVをマップ回転の逆方向に回転させてローカルUV内かを確認
	Vector2 targetUV = WorldToMinimapUV(m_MapData, targetWorldPos);
	Vector2 playerUV = Vector2(m_PlayerTexX, m_PlayerTexY);
	Vector2 dt = targetUV - playerUV;
	float cosInv = std::cos(-mapRotAngle);
	float sinInv = std::sin(-mapRotAngle);
	Vector2 dtRot;
	dtRot.x = dt.x * cosInv - dt.y * sinInv;
	dtRot.y = dt.x * sinInv + dt.y * cosInv;
	Vector2 rotatedTargetUV = playerUV + dtRot;
	float tLocalU = (rotatedTargetUV.x - uvOffset.x) / uvScale;
	float tLocalV = (rotatedTargetUV.y - uvOffset.y) / uvScale;
	if (tLocalU > 0.0f && tLocalU < 1.0f && tLocalV > 0.0f && tLocalV < 1.0f)
		return;

	// 5. プレイヤー前方を「上」としたときのターゲット角度
	//    cross.y = fwd.z * dir.x - fwd.x * dir.z（右が+）
	float crossY = playerFwd.z * toTarget.x - playerFwd.x * toTarget.z;
	float dotVal = playerFwd.x * toTarget.x + playerFwd.z * toTarget.z;
	float angle = std::atan2(crossY, dotVal);

	// 6. プレイヤーアイコンのスクリーン座標
	float playerLocalU = std::clamp((m_PlayerTexX - uvOffset.x) / uvScale, 0.0f, 1.0f);
	float playerLocalV = std::clamp((m_PlayerTexY - uvOffset.y) / uvScale, 0.0f, 1.0f);
	float playerScreenX = mapCX + (playerLocalU - 0.5f) * mapSize;
	float playerScreenY = mapCY + (0.5f - playerLocalV) * mapSize;

	// 7. 矢印スクリーン座標（上=前方、右=+X）
	const float edgeRadius = mapSize * kDirEdgeRate;
	float arrowX = playerScreenX + std::sin(angle) * edgeRadius;
	float arrowY = playerScreenY + std::cos(angle) * edgeRadius;

	// 8. 描画
	auto context = Renderer::GetDeviceContext();
	VisualObject::m_Shader.SetGPU();
	Renderer::SetBlendState(BS_ALPHABLEND);
	m_VertexBuffer.SetGPU();
	m_IndexBuffer.SetGPU();

	Matrix arrowWorld = Matrix::CreateScale(kDirArrowSize, kDirArrowSize, 1.0f)
		* Matrix::CreateRotationZ(angle)
		* Matrix::CreateTranslation(arrowX, arrowY, 0.5f);
	Renderer::SetWorldMatrix(&arrowWorld);
	m_TexDirArrow.SetGPU();
	context->DrawIndexed(6, 0, 0);

	Matrix iconWorld = Matrix::CreateScale(kDirIconSize, kDirIconSize, 1.0f)
		* Matrix::CreateTranslation(arrowX, arrowY, 0.5f);
	Renderer::SetWorldMatrix(&iconWorld);
	if (showGoal)
		m_TexDirGoal.SetGPU();
	else
		m_TexDirKey.SetGPU();
	context->DrawIndexed(6, 0, 0);
}
