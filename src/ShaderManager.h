#pragma once

#include "Shader.h"

using Microsoft::WRL::ComPtr;

struct ShaderData {
	ComPtr<ID3D11VertexShader> vertexShader;
	ComPtr<ID3D11PixelShader>  pixelShader;
	ComPtr<ID3D11InputLayout>  inputLayout;
};


class ShaderManager
{
public:

	static ShaderManager& GetInstance();

	std::shared_ptr<ShaderData> GetShader(const std::string& vs,const std::string& ps);

	// キャッシュをクリア
	void ClearCache();

	// キャッシュの統計情報を取得
	size_t GetCacheSize() const { return m_shaderCache.size(); }

private:

	ShaderManager() = default;
	~ShaderManager() = default;
	ShaderManager(const ShaderManager&) = delete;
	ShaderManager& operator=(const ShaderManager&) = delete;

	// シェーダーを実際に作成する
	std::shared_ptr<ShaderData> CreateShader(const std::string& vsPath, const std::string& psPath);

	// キャッシュキーを生成
	std::string GenerateCacheKey(const std::string& vsPath, const std::string& psPath);

	// シェーダーキャッシュ（キー：ファイルパス、値：シェーダーデータ）
	std::unordered_map<std::string, std::shared_ptr<ShaderData>> m_shaderCache;
};

