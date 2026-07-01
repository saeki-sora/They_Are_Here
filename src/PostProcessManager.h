#pragma once

#include "Renderer.h"

// ポストプロセス全体を管理するシングルトン
// HDRシーンRTへの描画結果に SSAO / ブルーム / ボリュメトリックライト /
// トーンマップ / カラーグレード / FXAA を適用してバックバッファへ出力する
class PostProcessManager
{
public:

	static PostProcessManager& GetInstance();

	void Init();
	void Uninit();
	void Update(float deltaTime);

	// 現在のグラフィック設定をJSONファイルへ保存する
	void SaveSettings(const std::string& filePath);
	// JSONファイルからグラフィック設定を読み込む（無ければ既定値を維持する）
	void LoadSettings(const std::string& filePath);

	// 3D描画完了後に呼び、ポストプロセスチェーンを実行してバックバッファへ出力する
	void Execute();

	// ImGuiデバッグパネルの中身を描画する
	void DrawImGui();

	// ポストプロセス全体が有効か
	bool IsEnabled() const;

	// HDRシーンレンダーターゲット（Renderer::Begin から参照する）
	ID3D11RenderTargetView* GetSceneRTV() const;

private:

	PostProcessManager() = default;
	~PostProcessManager() = default;
	PostProcessManager(const PostProcessManager&) = delete;
	PostProcessManager& operator=(const PostProcessManager&) = delete;

	// ブルームのMip段数
	static constexpr int BLOOM_MIP_COUNT = 3;
	// ボリュメトリックライトの最大数（HLSLと合わせる）
	static constexpr int MAX_VOLUMETRIC_LIGHTS = 8;

	// ポストプロセス用定数バッファ（HLSL: PostProcessBuffer b9 と一致させる）
	struct POST_PROCESS_BUFFER
	{
		DirectX::SimpleMath::Matrix InvViewProj;      // ビュープロジェクション逆行列
		DirectX::SimpleMath::Matrix InvProjection;    // プロジェクション逆行列
		DirectX::SimpleMath::Matrix Projection;       // プロジェクション行列
		DirectX::SimpleMath::Vector4 ScreenInfo;      // xy:出力先サイズ zw:1/出力先サイズ
		DirectX::SimpleMath::Vector4 ToneParams;      // x:露出 y:バイパス z:彩度 w:時間
		DirectX::SimpleMath::Vector4 BloomParams;     // x:閾値 y:ソフトニー z:強度 w:未使用
		DirectX::SimpleMath::Vector4 BloomWeights;    // xyz:Mip毎の重み w:未使用
		DirectX::SimpleMath::Vector4 BlurDir;         // xy:ブラー方向(テクセル) zw:未使用
		DirectX::SimpleMath::Vector4 VignetteParams;  // x:ビネット強度 y:形状べき z:グレイン強度 w:未使用
		DirectX::SimpleMath::Vector4 ColorTint;       // rgb:ティント色 w:強度
		DirectX::SimpleMath::Vector4 SSAOParams;      // x:半径 y:強度 z:バイアス w:適用フラグ
		DirectX::SimpleMath::Vector4 VolumetricParams;// x:散乱強度 y:ステップ数 z:最大距離 w:適用フラグ
		DirectX::SimpleMath::Vector4 CameraParams;    // x:ニア y:ファー zw:未使用
		DirectX::SimpleMath::Vector4 CameraPos;       // xyz:カメラ位置 w:未使用
	};

	// ボリュメトリックライト用定数バッファ（HLSL: VolumetricLightBuffer b10 と一致させる）
	struct VOLUMETRIC_LIGHT_BUFFER
	{
		DYNAMIC_LIGHT_DATA Lights[MAX_VOLUMETRIC_LIGHTS]; // カメラ近傍の抽出ライト
	};

	// オフスクリーンRT一式（テクスチャ・RTV・SRV）
	struct RenderTarget
	{
		Microsoft::WRL::ComPtr<ID3D11Texture2D> Tex;          // テクスチャ本体
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> RTV;   // レンダーターゲットビュー
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SRV; // シェーダーリソースビュー
		UINT Width = 0;  // 幅
		UINT Height = 0; // 高さ
	};

	// RT作成ヘルパー
	bool CreateRenderTarget(RenderTarget& rt, UINT width, UINT height, DXGI_FORMAT format);
	// ピクセルシェーダーのロードヘルパー
	bool LoadPixelShader(Microsoft::WRL::ComPtr<ID3D11PixelShader>& ps, const char* fileName);

	// 定数バッファの共通部分（行列・カメラ・各パラメータ）を更新する
	void UpdateConstantBuffer();
	// 定数バッファをGPUへ転送する
	void PushConstantBuffer();
	// フルスクリーン三角形を出力先RTへ描画する
	void DrawFullScreen(ID3D11RenderTargetView* rtv, ID3D11PixelShader* ps, UINT width, UINT height);

	// 各エフェクトのパス実行
	void ExecuteSSAO();
	void ExecuteBloom();
	void ExecuteVolumetric();
	void ExecuteComposite();

	// 描画ステートを通常描画用へ復帰させる
	void RestoreRenderState();

	// --- D3Dリソース ---
	Microsoft::WRL::ComPtr<ID3D11VertexShader> m_FullScreenVS;   // フルスクリーン三角形VS
	Microsoft::WRL::ComPtr<ID3D11PixelShader> m_CompositePS;     // 最終合成PS
	Microsoft::WRL::ComPtr<ID3D11PixelShader> m_FXAAPS;          // FXAA PS
	Microsoft::WRL::ComPtr<ID3D11PixelShader> m_BrightExtractPS; // 輝度抽出PS
	Microsoft::WRL::ComPtr<ID3D11PixelShader> m_DownsamplePS;    // ダウンサンプルPS
	Microsoft::WRL::ComPtr<ID3D11PixelShader> m_GaussBlurPS;     // ガウスブラーPS
	Microsoft::WRL::ComPtr<ID3D11PixelShader> m_SSAOPS;          // SSAO PS
	Microsoft::WRL::ComPtr<ID3D11PixelShader> m_BilateralBlurPS; // バイラテラルブラーPS
	Microsoft::WRL::ComPtr<ID3D11PixelShader> m_VolumetricPS;    // ボリュメトリックPS

	Microsoft::WRL::ComPtr<ID3D11Buffer> m_PostBuffer;       // ポスト用定数バッファ(b9)
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_VolumetricBuffer; // ボリュメトリック用定数バッファ(b10)

	Microsoft::WRL::ComPtr<ID3D11SamplerState> m_LinearClampSampler; // リニアクランプサンプラー(s0)
	Microsoft::WRL::ComPtr<ID3D11SamplerState> m_PointClampSampler;  // ポイントクランプサンプラー(s1)

	RenderTarget m_SceneRT;                         // HDRシーンRT（フル解像度 RGBA16F）
	RenderTarget m_LDRRT;                           // トーンマップ後の中間RT（FXAA入力用）
	RenderTarget m_BloomExtractRT;                  // 輝度抽出RT（1/2解像度）
	RenderTarget m_BloomRT[BLOOM_MIP_COUNT];        // ブルームMipチェーン（1/4, 1/8, 1/16）
	RenderTarget m_BloomWorkRT[BLOOM_MIP_COUNT];    // ブルームブラー作業用
	RenderTarget m_SSAORT;                          // SSAO結果RT（1/2解像度 R8）
	RenderTarget m_SSAOWorkRT;                      // SSAOブラー作業用
	RenderTarget m_VolumetricRT;                    // ボリュメトリック結果RT（1/2解像度）
	RenderTarget m_VolumetricWorkRT;                // ボリュメトリックブラー作業用

	// --- 状態 ---
	POST_PROCESS_BUFFER m_CBData{}; // 定数バッファのCPU側コピー
	bool m_IsInitialized = false;   // 初期化済みか
	float m_Time = 0.0f;            // 経過時間（グレインのシード用）

	// --- ImGuiで調整するパラメータ ---
	bool m_Enabled = true;          // ポストプロセス全体の有効/無効
	bool m_TonemapBypass = false;   // トーンマップをバイパスする（デバッグ用）
	bool m_BloomEnabled = true;     // ブルーム有効
	bool m_FXAAEnabled = true;      // FXAA有効
	bool m_SSAOEnabled = true;      // SSAO有効
	bool m_VolumetricEnabled = true;// ボリュメトリックライト有効
	bool m_SSAODebugView = false;   // AOバッファを直接表示する（デバッグ用）

	float m_Exposure = 1.3f;          // 露出
	float m_Saturation = 0.92f;       // 彩度
	float m_BloomThreshold = 0.75f;   // ブルーム輝度閾値
	float m_BloomKnee = 0.35f;        // ブルームソフトニー
	float m_BloomIntensity = 0.8f;    // ブルーム強度
	float m_VignetteIntensity = 0.45f;// ビネット強度
	float m_VignettePower = 1.4f;     // ビネット形状べき
	float m_GrainIntensity = 0.035f;  // フィルムグレイン強度
	float m_TintStrength = 0.35f;     // カラーティント強度
	DirectX::SimpleMath::Vector3 m_TintColor{ 0.82f, 0.85f, 1.1f }; // ティント色（青紫寄り）
	float m_SSAORadius = 35.0f;       // SSAO半径（ビュー空間）
	float m_SSAOIntensity = 1.6f;     // SSAO強度
	float m_SSAOBias = 0.025f;        // SSAOバイアス
	float m_VolumetricIntensity = 1.2f; // ボリュメトリック散乱強度
	int m_VolumetricSteps = 20;       // ボリュメトリックステップ数
	float m_VolumetricMaxDist = 1200.0f; // ボリュメトリックレイ最大距離
};
