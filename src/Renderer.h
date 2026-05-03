#pragma once


#include "SkinnedModel.h"
//#include "imgui.h"
//#include "imgui_impl_win32.h"
//#include "imgui_impl_dx11.h"
//#include"ImGUI_Manager.h"

//外部ライブラリ
#pragma comment(lib,"directxtk.lib")
#pragma comment(lib,"d3d11.lib")

class Shader;
class SkinnedModel;
class SkinnedMesh;


// ３Ｄ頂点データ
struct VERTEX_3D
{
	DirectX::SimpleMath::Vector3 position;
	DirectX::SimpleMath::Vector3 normal;
	DirectX::SimpleMath::Color   color;
	DirectX::SimpleMath::Vector2 uv;

};

// スキニング頂点データ
struct VERTEX_SKINNED
{
	DirectX::SimpleMath::Vector3 position;
	DirectX::SimpleMath::Vector3 normal;
	DirectX::SimpleMath::Color   color;
	DirectX::SimpleMath::Vector2 uv;

	uint8_t boneIndex[4] = { 0, 0, 0, 0 };
	float   boneWeight[4] = { 1.0f, 0.0f, 0.0f, 0.0f };
};


// ブレンドステート
enum EBlendState {
	BS_NONE = 0,							// 半透明合成無し
	BS_ALPHABLEND,							// 半透明合成
	BS_ADDITIVE,							// 加算合成
	BS_SUBTRACTION,							// 減算合成
	MAX_BLENDSTATE,
	BS_OPAQUE		// 不透明
};

//平行光源
struct LIGHT
{
	BOOL Enable;//有効無効のフラグ
	DirectX::SimpleMath::Vector3 CameraPos;
	DirectX::SimpleMath::Vector4 Direction;//光の方向
	DirectX::SimpleMath::Color   Diffuse;//強さと色
	DirectX::SimpleMath::Color   Ambient;//環境光の強さと色
};


constexpr int MAX_POINT_LIGHTS = 32; // 最大点光源数 (HLSLと合わせる)

// 点光源単体のデータ
struct DYNAMIC_LIGHT_DATA
{
	DirectX::SimpleMath::Vector4 Position; // w:Range (範囲)
	DirectX::SimpleMath::Vector4 Direction;// xyz:向き, w:スポットライトの角度
	DirectX::SimpleMath::Color   Color;    // w:Enable (有効フラグ >0 で有効)
};

// 定数バッファ全体
struct LIGHT_CONSTANT_BUFFER
{
	LIGHT GlobalLight; // 既存の平行光源
	DYNAMIC_LIGHT_DATA Lights[MAX_POINT_LIGHTS]; // 点光源配列
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
	static ID3D11Buffer* m_WorldInverseTransposeBuffer;//ワールド逆転置行列
	static ID3D11Buffer* m_ViewBuffer;//ビュー行列
	static ID3D11Buffer* m_ProjectionBuffer;//プロジェクション行列

	static ID3D11Buffer* m_LightBuffer;//ライト情報
	static ID3D11Buffer* m_MaterialBuffer;//マテリアル情報
	static ID3D11Buffer* m_TextureBuffer;//マテリアル情報

	static ID3D11Buffer* m_SkinningBuffer;//スキニング情報

	static ID3D11Buffer* m_UVAdjustBuffer;//UV情報

	static ID3D11DepthStencilState* m_DepthStateEnable;//深度ステンシルステート
	static ID3D11DepthStencilState* m_DepthStateDisable;//深度ステンシルステート

	static ID3D11BlendState* m_BlendState[MAX_BLENDSTATE]; // ブレンド ステート;
	static ID3D11BlendState* m_BlendStateATC;

	static ID3D11SamplerState* m_SamplerState;//サンプラーステート

	static DirectX::SimpleMath::Matrix m_ViewMatrix;        // ビュー行列のコピー
	static DirectX::SimpleMath::Matrix m_ProjectionMatrix;  // プロジェクション行列のコピー
	static DirectX::SimpleMath::Matrix m_WorldMatrix;       // ワールド行列のコピー

	static Shader m_SkinnedShader;

	static LIGHT_CONSTANT_BUFFER m_LightData;//現在のライト情報を保持する変数

	// シャドウマップ用リソース
	static ID3D11Texture2D*          m_ShadowMapTex;
	static ID3D11DepthStencilView*   m_ShadowDSV;
	static ID3D11ShaderResourceView* m_ShadowSRV;
	static ID3D11SamplerState*       m_ShadowSampler;
	static ID3D11Buffer*             m_ShadowBuffer;
	static ID3D11RasterizerState*    m_ShadowRS;
	static Shader                    m_ShadowStaticShader;   // スタティックメッシュ用
	static Shader                    m_ShadowSkinnedShader;  // スキニングメッシュ用

	//ライト情報をGPUへ送る内部関数
	static void PushLightBuffer();

public:

	static void Init();
	static void Draw(const SkinnedModel& model, const DirectX::SimpleMath::Matrix& world);
	static void Uninit();
	static void Begin();
	static void End();

	// シャドウパス
	static void BeginShadowPass();
	static void EndShadowPass();
	static void SetShadowStaticShader();
	static void SetShadowSkinnedShader();
	static void DrawShadow(const SkinnedModel& model, const DirectX::SimpleMath::Matrix& world);

	//セッター
	static void SetDepthEnable(bool Enable);
	static void SetATCEnable(bool Enable);
	static void SetWorldViewProjection2D();
	static void SetWorldMatrix(DirectX::SimpleMath::Matrix* WorldMatrix);
	static void SetViewMatrix(DirectX::SimpleMath::Matrix* ViewMatrix);
	static void SetProjectionMatrix(DirectX::SimpleMath::Matrix* ProjectionMatrix);
	static void BindSampler();
	static void BindLitCommonCB();


	//ゲッター
	static ID3D11Device* GetDevice(void) { return m_Device; }
	static ID3D11DeviceContext* GetDeviceContext(void) { return m_DeviceContext; }
	static DirectX::SimpleMath::Matrix GetViewMatrix(void){ return m_ViewMatrix; }
	static DirectX::SimpleMath::Matrix GetProjectionMatrix(){ return m_ProjectionMatrix; }
	static ID3D11DepthStencilView* GetDepthStencilView() { return m_DepthStencilView; }
	static ID3D11RenderTargetView* GetRenderTargetView() { return m_RenderTargetView; }
	static ID3D11Buffer* GetWorldBuffer() { return m_WorldBuffer; }
	static ID3D11Buffer* GetViewBuffer() { return m_ViewBuffer; }
	static ID3D11Buffer* GetProjectionBuffer() { return m_ProjectionBuffer; }


	static void CreateVertexShader(ID3D11VertexShader** VertexShader, ID3D11InputLayout** VertexLayout, const char* FileName);
	static void CreatePixelShader(ID3D11PixelShader** PixelShader, const char* FileName);

	static void SetLight(LIGHT Light);
	static void SetMaterial(MATERIAL Material);
	static void SetUV(float u, float v, float uw, float vh);

	static void BindVS(ID3D11VertexShader* vs);// シェーダーのバインド
	static void BindPS(ID3D11PixelShader* ps);
	static void BindIL(ID3D11InputLayout* il);
	static void ResetStateCache(); // シーン切替などでキャッシュ初期化

	static void SetSkinningMatrices(const std::vector<DirectX::SimpleMath::Matrix>& boneMatrices);// スキニング行列の設定

	//=============================================================================
	// ブレンド ステート設定
	//=============================================================================
	static void SetBlendState(int nBlendState)
	{
		if (nBlendState >= 0 && nBlendState < MAX_BLENDSTATE)
		{
			float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
			m_DeviceContext->OMSetBlendState(m_BlendState[nBlendState], blendFactor, 0xffffffff);
		}
	}

	// ==========================================================
	// 光源操作メソッド
	// ==========================================================
	// 光源を追加し、IDを返す（失敗時は-1）
	static int AddPointLight(const DirectX::SimpleMath::Vector3& pos, float range, const DirectX::SimpleMath::Color& color);
	static int AddSpotLight(const DirectX::SimpleMath::Vector3& pos, const DirectX::SimpleMath::Vector3& dir, float range, float angleDeg, const DirectX::SimpleMath::Color& color);

	// 光源を更新する
	static void UpdatePointLight(int id, const DirectX::SimpleMath::Vector3& pos, float range, const DirectX::SimpleMath::Color& color);
	static void UpdateSpotLight(int id, const DirectX::SimpleMath::Vector3& pos, const DirectX::SimpleMath::Vector3& dir, float range, float angleDeg, const DirectX::SimpleMath::Color& color);

	// 光源を削除する
	static void RemoveLight(int id);

	// 全ての光源を削除する
	static void ClearLights();
};