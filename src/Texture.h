#pragma once

using Microsoft::WRL::ComPtr;

//-----------------------------------------------------------------------------
//Textureクラス
//-----------------------------------------------------------------------------
class Texture
{
	std::string m_texname{}; // ファイル名
	ComPtr<ID3D11ShaderResourceView> m_srv{}; // シェーダーリソースビュー
	ComPtr<ID3D11ShaderResourceView> m_normalSrv{}; // ノーマルマップのSRV（_nファイル or 自動生成）

	int m_width = 0; // 幅
	int m_height = 0; // 高さ
	int m_bpp = 0; // BPP

	// 平坦なノーマルマップ（ノーマルマップ未所持テクスチャの共通フォールバック）
	static ComPtr<ID3D11ShaderResourceView> s_FlatNormalSrv;

	// アルベドのピクセルデータからSobelフィルタでノーマルマップを生成する
	bool GenerateNormalMap(const unsigned char* pixels, int width, int height);
	// "<名前>_n.<拡張子>" のノーマルマップ画像があればロードする
	bool LoadNormalMapFile(const std::string& albedoFilename);
	// 平坦ノーマルマップを取得する（未作成なら生成）
	static ID3D11ShaderResourceView* GetFlatNormalSRV();

public:

	bool Load(const std::string& filename);
	bool LoadFromMemory(const unsigned char* data,int len);

	void SetGPU();
	void SetWidth(int width);
	void SetHeight(int height);

	int GetWidth() const { return m_width; }
	int GetHeight() const { return m_height; }

	ID3D11ShaderResourceView* GetSRV() const { return m_srv.Get(); }
	ID3D11ShaderResourceView* const* GetSRVAddress() const { return m_srv.GetAddressOf(); }


	void CreateFromSRV(ComPtr<ID3D11ShaderResourceView> srv);
};