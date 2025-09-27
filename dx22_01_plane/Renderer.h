#pragma once

#include	<d3d11.h>
#include	<DirectXMath.h>
#include	<SimpleMath.h>
#include	<io.h>
#include	<string>
#include	<vector>

//外部ライブラリ
#pragma comment(lib,"directxtk.lib")
#pragma comment(lib,"d3d11.lib")

// ３Ｄ頂点データ
struct VERTEX_3D
{
	DirectX::SimpleMath::Vector3 position;
	DirectX::SimpleMath::Vector3 normal;
	DirectX::SimpleMath::Color color;
	DirectX::SimpleMath::Vector2 uv;
};

// ブレンドステート
enum EBlendState {
	BS_NONE = 0,							// 半透明合成無し
	BS_ALPHABLEND,							// 半透明合成
	BS_ADDITIVE,							// 加算合成
	BS_SUBTRACTION,							// 減算合成
	MAX_BLENDSTATE
};

//平行光源
struct LIGHT
{
	BOOL Enable;//有効無効のフラグ
	BOOL Dummy[3];//アラインメントの都合でダミー
	DirectX::SimpleMath::Vector4 Direction;//光の方向
	DirectX::SimpleMath::Color   Diffuse;//強さと色
	DirectX::SimpleMath::Color   Ambient;//環境光の強さと色
};

//メッシュ
struct SUBSET
{
	std::string MtrlName;//マテリアル名
	unsigned int IndexNum = 0;//インデックス数
	unsigned int VertexNum = 0;//頂点数
	unsigned int IndexBase = 0;//開始インデックス
	unsigned int VertexBase = 0;//開始頂点
	unsigned int MaterialIdx = 0;//マテリアルインデックス

};

//マテリアル
struct MATERIAL
{
	DirectX::SimpleMath::Color Ambient;//環境光の強さと色
	DirectX::SimpleMath::Color Diffuse;//拡散光の強さと色
	DirectX::SimpleMath::Color Specular;//鏡面光の強さと色
	DirectX::SimpleMath::Color Emission;//発光

	float Shiness;//光沢度
	BOOL TextureEnable;//テクスチャの有無
	BOOL Dummy[2];//アラインメントの都合でダミー
};


//-----------------------------------------------------------------------------
//Rendererクラス
//-----------------------------------------------------------------------------
class Renderer
{
private:

	static D3D_FEATURE_LEVEL       m_FeatureLevel;//Direct3Dの機能レベル

	static ID3D11Device* m_Device;//Direct3Dデバイス
	static ID3D11DeviceContext* m_DeviceContext;//DeviceContext
	static IDXGISwapChain* m_SwapChain;//スワップチェーン
	static ID3D11RenderTargetView* m_RenderTargetView;//レンダーターゲットビュー
	static ID3D11RenderTargetView* m_RenderTargetView_Mirror;//レンダーターゲットビュー

	static ID3D11DepthStencilView* m_DepthStencilView;//深度ステンシルビュー

	static ID3D11Buffer* m_WorldBuffer;//ワールド行列
	static ID3D11Buffer* m_ViewBuffer;//ビュー行列
	static ID3D11Buffer* m_ProjectionBuffer;//プロジェクション行列

	static ID3D11Buffer* m_LightBuffer;//ライト情報
	static ID3D11Buffer* m_MaterialBuffer;//マテリアル情報
	static ID3D11Buffer* m_TextureBuffer;//マテリアル情報


	static ID3D11DepthStencilState* m_DepthStateEnable;//深度ステンシルステート
	static ID3D11DepthStencilState* m_DepthStateDisable;//深度ステンシルステート

	static ID3D11BlendState* m_BlendState[MAX_BLENDSTATE]; // ブレンド ステート;
	static ID3D11BlendState* m_BlendStateATC;

	static DirectX::SimpleMath::Matrix m_ViewMatrix;        // ビュー行列のコピー
	static DirectX::SimpleMath::Matrix m_ProjectionMatrix;  // プロジェクション行列のコピー
	static DirectX::SimpleMath::Matrix m_WorldMatrix;       // ワールド行列のコピー

public:

	static void Init();
	static void Uninit();
	static void Begin();
	static void End();

	static void SetDepthEnable(bool Enable);

	static void SetATCEnable(bool Enable);

	static void SetWorldViewProjection2D();
	static void SetWorldMatrix(DirectX::SimpleMath::Matrix* WorldMatrix);
	static void SetViewMatrix(DirectX::SimpleMath::Matrix* ViewMatrix);
	static void SetProjectionMatrix(DirectX::SimpleMath::Matrix* ProjectionMatrix);

	static ID3D11Device* GetDevice(void) { return m_Device; }
	static ID3D11DeviceContext* GetDeviceContext(void) { return m_DeviceContext; }
	static DirectX::SimpleMath::Matrix GetViewMatrix(void){ return m_ViewMatrix; }
	static DirectX::SimpleMath::Matrix GetProjectionMatrix(){ return m_ProjectionMatrix; }

	static ID3D11DepthStencilView* GetDepthStencilView() { return m_DepthStencilView; }

	static void CreateVertexShader(ID3D11VertexShader** VertexShader, ID3D11InputLayout** VertexLayout, const char* FileName);
	static void CreatePixelShader(ID3D11PixelShader** PixelShader, const char* FileName);

	static void SetLight(LIGHT Light);
	static void SetMaterial(MATERIAL Material);
	static void SetUV(float u, float v, float uw, float vh);

	static void BindVS(ID3D11VertexShader* vs);// シェーダーのバインド
	static void BindPS(ID3D11PixelShader* ps);
	static void BindIL(ID3D11InputLayout* il);
	static void ResetStateCache(); // シーン切替などでキャッシュ初期化

	//=============================================================================
	// ブレンド ステート設定
	//=============================================================================
	static void SetBlendState(int nBlendState)
	{
		if (nBlendState >= 0 && nBlendState < MAX_BLENDSTATE) {
			float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
			m_DeviceContext->OMSetBlendState(m_BlendState[nBlendState], blendFactor, 0xffffffff);
		}
	}
};