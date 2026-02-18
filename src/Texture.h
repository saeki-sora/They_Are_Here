#pragma once

using Microsoft::WRL::ComPtr;

//-----------------------------------------------------------------------------
//Textureクラス
//-----------------------------------------------------------------------------
class Texture
{
	std::string m_texname{}; // ファイル名
	ComPtr<ID3D11ShaderResourceView> m_srv{}; // シェーダーリソースビュー

	int m_width; // 幅
	int m_height; // 高さ
	int m_bpp; // BPP

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