#pragma once
#include "json.hpp"


using json = nlohmann::json;

class ConfigManager
{
public:

	// シングルトンインスタンス取得
    static ConfigManager& GetInstance()
    {
        static ConfigManager instance;
        return instance;
    }

    // JSONデータの読み込み
    void Load(const std::string& filePath);

	// JSONデータの取得（読み取り専用）
    const json& GetData() const { return m_Data; }

	// JSONデータの取得（書き込み可能・ImGuiのスライダーから直接値を書き換えるために使う）
	json& GetDataMutable() { return m_Data; }

	// 現在のJSONデータをファイルに書き出す（"Apply"後に永続化したいときに呼ぶ）
	void Save(const std::string& filePath);

private:

    ConfigManager() = default;
    ~ConfigManager() = default;
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    json m_Data;
};